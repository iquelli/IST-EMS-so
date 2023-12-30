#include "server.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "producer-consumer.h"
#include "workers.h"

// Requests queue
static pc_queue_t *queue;

// Workers
static pthread_t *workers;

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return EXIT_FAILURE;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  char *server_pipename = argv[1];

  // Intializes server, creates worker threads
  if (server_init(server_pipename, state_access_delay_us)) {
    return EXIT_FAILURE;
  }

  int server_fd = open(server_pipename, O_RDONLY);
  if (server_fd < 0) {
    fprintf(stderr, "Failed to open server pipe.\n");
    server_close(server_pipename, server_fd);
    return EXIT_FAILURE;
  }

  while (1) {
    char op_code;

    if (pipe_parse(server_fd, &op_code, sizeof(char))) {
      fprintf(stderr, "Failed to read op code from server pipe: %s\n", server_pipename);
      server_close(server_pipename, server_fd);
      return EXIT_FAILURE;
    }

    if (op_code != OP_CODE_SETUP_REQUEST) {
      // in case it didn't receive a connection request, dismiss
      continue;
    }

    // Reads rest of message and adds client to queue
    if (receive_connection(server_fd)) {
      server_close(server_pipename, server_fd);
      return EXIT_FAILURE;
    }

    // To avoid having active wait for another process to open the pipe.
    int tmp_pipe = open(server_pipename, O_RDONLY);
    if (tmp_pipe < 0) {
      fprintf(stderr, "Failed to open server pipe");
      server_close(server_pipename, server_fd);
      return EXIT_FAILURE;
    }
    if (close(tmp_pipe) < 0) {
      fprintf(stderr, "Failed to close server pipe");
      server_close(server_pipename, server_fd);
      return EXIT_FAILURE;
    }
  }

  server_close(server_pipename, server_fd);
  return 0;
}

int server_init(char *server_pipename, unsigned int delay_us) {
  // Initialize EMS state.
  if (ems_init(delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return EXIT_FAILURE;
  }

  // Create producer-consumer queue
  queue = (pc_queue_t *)malloc(sizeof(pc_queue_t));
  if (queue == NULL) {
    ems_terminate();
    return EXIT_FAILURE;
  }
  if (pcq_create(queue, MAX_SESSION_COUNT) != 0) {
    ems_terminate();
    free(queue);
    return EXIT_FAILURE;
  }

  // Initialize worker threads
  workers = malloc(MAX_SESSION_COUNT * sizeof(pthread_t));
  if (workers == NULL) {
    ems_terminate();
    pcq_destroy(queue);
    free(queue);
    return EXIT_FAILURE;
  }
  if (workers_init() != 0) {
    ems_terminate();
    pcq_destroy(queue);
    free(queue);
    free(workers);
    return EXIT_FAILURE;
  }

  // Initialize the server
  if ((unlink(server_pipename) != 0 && errno != ENOENT) || mkfifo(server_pipename, 0777) < 0) {
    fprintf(stderr, "Failed to initialize server.\n");
    ems_terminate();
    pcq_destroy(queue);
    free(queue);
    free(workers);
    return EXIT_FAILURE;
  }
  fprintf(stdout, "The server has been initialized with pipename: %s.\n", server_pipename);

  return 0;
}

int receive_connection(int server_pipe_fd) {
  client_t *client = (client_t *)malloc(sizeof(client_t));

  char client_resquest_pipename[CLIENT_PIPE_MAX_LEN];
  if (pipe_parse(server_pipe_fd, &client_resquest_pipename, CLIENT_PIPE_MAX_LEN * sizeof(char))) {
    fprintf(stderr, "Failed to read data out of server pipe.\n");
    free(client);
    return 1;
  }
  strcpy(client->request_pipename, client_resquest_pipename);

  char client_response_pipename[CLIENT_PIPE_MAX_LEN];
  if (pipe_parse(server_pipe_fd, &client_response_pipename, CLIENT_PIPE_MAX_LEN * sizeof(char))) {
    fprintf(stderr, "Failed to read data out of server pipe.\n");
    free(client);
    return 1;
  }
  strcpy(client->response_pipename, client_response_pipename);

  if (pcq_enqueue(queue, (void *)client)) {
    fprintf(stderr, "Failed to queue up client.\n");
    free(client);
    return 1;
  }

  return 0;
}

void *process_incoming_requests(void *arg) {
  int session_id = (int)(intptr_t)arg;

  client_t *client;
  while (1) {
    // Dequeues a client to process.
    client = (client_t *)pcq_dequeue(queue);
    if (client == NULL) {
      continue;
    }

    ems_setup_handler(session_id, client);

    // Open client pipe to read the op codes
    int request_fd = open(client->request_pipename, O_RDONLY);
    char op_code;

    while (1) {
      if (pipe_parse(request_fd, &op_code, sizeof(char))) {
        break;  // failed to get op code
      }

      switch (op_code) {
        case OP_CODE_CREATE_REQUEST: {
          // Args
          unsigned int event_id;
          size_t num_rows, num_cols;

          if (pipe_parse(request_fd, &event_id, sizeof(unsigned int)) ||
              pipe_parse(request_fd, &num_rows, sizeof(size_t)) || pipe_parse(request_fd, &num_cols, sizeof(size_t))) {
            break;  // failed to get args
          }

          if (ems_create_handler(client, event_id, num_rows, num_cols)) {
            fprintf(stderr, "Failed to perform ems_create for client.\n");
          }
          break;
        }

        case OP_CODE_RESERVE_REQUEST: {
          // Args
          unsigned int event_id;
          size_t num_seats, *xs, *ys;

          if (pipe_parse(request_fd, &event_id, sizeof(unsigned int)) ||
              pipe_parse(request_fd, &num_seats, sizeof(size_t))) {
            break;  // failed to get args
          }

          xs = malloc(sizeof(size_t) * num_seats);
          ys = malloc(sizeof(size_t) * num_seats);

          if (!xs || !ys) {
            fprintf(stderr, "Memory allocation failed.\n");
            free(xs);
            free(ys);
            break;
          }
          for (size_t i = 0; i < num_seats; i++) {
            if (pipe_parse(request_fd, &xs[i], sizeof(size_t))) {
              free(xs);
              free(ys);
              break;  // failed to get args
            }
          }
          for (size_t i = 0; i < num_seats; i++) {
            if (pipe_parse(request_fd, &ys[i], sizeof(size_t))) {
              free(xs);
              free(ys);
              break;  // failed to get args
            }
          }

          if (ems_reserve_handler(client, event_id, num_seats, xs, ys)) {
            fprintf(stderr, "Failed to perform ems_reserve for a client.\n");
          }

          free(xs);
          free(ys);
          break;
        }
        case OP_CODE_SHOW_REQUEST: {
          unsigned int event_id;

          if (pipe_parse(request_fd, &event_id, sizeof(unsigned int))) {
            break;  // failed to get args
          }

          if (ems_show_handler(client, event_id)) {
            fprintf(stderr, "Failed to perform ems_show for a client.\n");
          }

          break;
        }
        case OP_CODE_LIST_REQUEST:
          ems_list_handler(client);
          break;
        case OP_CODE_QUIT_REQUEST:
          // Will leave loop in following if opening up the session for another client
          break;
        default:
          break;
      }

      if (op_code == OP_CODE_QUIT_REQUEST) {
        break;
      }

      // To avoid having active wait for another process to open the pipe.
      int tmp_pipe = open(client->request_pipename, O_RDONLY);
      if (tmp_pipe < 0) {
        fprintf(stderr, "Failed to open client request pipe");
        break;
      }
      if (close(tmp_pipe) < 0) {
        fprintf(stderr, "Failed to close client request pipe");
        break;
      }
    }

    close(request_fd);
    free(client);
  }
}

int server_close(char *server_pipename, int server_fd) {
  if (ems_terminate()) {
    fprintf(stderr, "Failed to destroy EMS\n");
    return EXIT_FAILURE;
  }

  pcq_destroy(queue);
  free(queue);
  free(workers);

  close(server_fd);
  unlink(server_pipename);

  return 0;
}

int workers_init() {
  for (int i = 0; i < MAX_SESSION_COUNT; i++) {  // TODO: only one session for now
    if (pthread_create(&workers[i], NULL, process_incoming_requests, (void *)(intptr_t)i) != 0) {
      return 1;
    }
  }
  return 0;
}