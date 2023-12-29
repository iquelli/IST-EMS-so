#include "workers.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "operations.h"

// Handlers

// TODO: these all should read the pipe and perform the intended action,
//       replying to client when necessary

int ems_setup_handler(int session_id, client_t *client) {
  // Initialize variables
  size_t response_len = sizeof(int);
  char response[response_len];
  size_t offset = 0;
  memset(response, 0, response_len);

  // Create message:
  // [session id (int)]
  create_message(response, &offset, &session_id, sizeof(int));

  // Connect to client pipe and send response
  int response_fd = open(client->response_pipename, O_WRONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  if (pipe_print(response_fd, &response, response_len)) {
    fprintf(stderr, "Failed to send response to setup.\n");
    close(response_fd);
    return 1;
  }

  close(response_fd);
  return 0;
}

int ems_quit_handler(client_t *client);

int ems_create_handler(client_t *client, unsigned int event_id, size_t num_rows, size_t num_cols) {
  size_t response_len = sizeof(int);
  char response[response_len];
  size_t offset = 0;
  memset(response, 0, response_len);

  // create event and send message
  int result = ems_create(event_id, num_rows, num_cols);

  // 0 to indicate it was created, 1 otherwise
  create_message(response, &offset, &result, sizeof(int));

  // Connect to client pipe and send response
  int response_fd = open(client->response_pipename, O_WRONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  if (pipe_print(response_fd, &response, response_len)) {
    fprintf(stderr, "Failed to send response to setup.\n");
    close(response_fd);
    return 1;
  }
  close(response_fd);

  return 0;
}

int ems_reserve_handler(client_t *client, unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys) {
  size_t response_len = sizeof(int);
  char response[response_len];
  size_t offset = 0;
  memset(response, 0, response_len);

  int result = ems_reserve(event_id, num_seats, xs, ys);

  create_message(response, &offset, &result, sizeof(int));

  // Connect to client pipe and send response
  int response_fd = open(client->response_pipename, O_WRONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  if (pipe_print(response_fd, &response, response_len)) {
    fprintf(stderr, "Failed to send response to setup.\n");
    close(response_fd);
    return 1;
  }
  close(response_fd);

  return 0;
}

int ems_show_handler(client_t *client);

int ems_list_handler(client_t *client);