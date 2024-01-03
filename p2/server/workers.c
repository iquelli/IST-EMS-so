#include "workers.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "operations.h"

// Handlers

int ems_setup_handler(int session_id, client_t *client) {
  // Initialize variables
  size_t response_len = sizeof(int);
  char response[response_len];
  size_t offset = 0;
  memset(response, 0, response_len);

  // [session id (int)]
  create_message(response, &offset, &session_id, sizeof(int));

  // Connect to client pipe and send response
  int response_fd = open(client->response_pipename, O_WRONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  if (pipe_print(response_fd, &response, response_len)) {
    fprintf(stderr, "Failed to send response.\n");
    close(response_fd);
    return 1;
  }

  close(response_fd);
  return 0;
}

int ems_create_handler(client_t *client, unsigned int event_id, size_t num_rows, size_t num_cols) {
  size_t response_len = sizeof(int);
  char response[response_len];
  size_t offset = 0;
  memset(response, 0, response_len);

  // create event and send message
  int result = ems_create(event_id, num_rows, num_cols);

  // [result (int)]
  create_message(response, &offset, &result, sizeof(int));

  // Connect to client pipe and send response
  int response_fd = open(client->response_pipename, O_WRONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  if (pipe_print(response_fd, &response, response_len)) {
    fprintf(stderr, "Failed to send response.\n");
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

  // [result (int)]
  create_message(response, &offset, &result, sizeof(int));

  // Connect to client pipe and send response
  int response_fd = open(client->response_pipename, O_WRONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  if (pipe_print(response_fd, &response, response_len)) {
    fprintf(stderr, "Failed to send response.\n");
    close(response_fd);
    return 1;
  }
  close(response_fd);

  return 0;
}

int ems_show_handler(client_t *client, unsigned int event_id) {
  size_t num_rows;
  size_t num_cols;
  unsigned int *seats;

  int result = get_event_info(event_id, &num_cols, &seats, &num_rows);

  size_t response_len = sizeof(int) + sizeof(size_t) + sizeof(size_t) + sizeof(unsigned int) * num_cols * num_rows;
  char response[response_len];
  size_t offset = 0;
  memset(response, 0, response_len);

  // [ result (int) ] | [ num_rows (size_t) ] | [ num_cols (size_t) ]
  // | [ seats[num_rows * num_cols] (unsigned int) ]
  create_message(response, &offset, &result, sizeof(int));
  if (result == 0) {
    create_message(response, &offset, &num_rows, sizeof(size_t));
    create_message(response, &offset, &num_cols, sizeof(size_t));
    for (size_t i = 0; i < num_cols * num_rows; i++) {
      create_message(response, &offset, &seats[i], sizeof(unsigned int));
    }
  }

  // Connect to client pipe and send response
  int response_fd = open(client->response_pipename, O_WRONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  if (pipe_print(response_fd, &response, response_len)) {
    fprintf(stderr, "Failed to send response.\n");
    close(response_fd);
    return 1;
  }
  close(response_fd);

  return 0;
}

int ems_list_handler(client_t *client) {
  size_t num_events;
  unsigned int *events;

  int result = get_events(&events, &num_events);

  size_t response_len = sizeof(int) + sizeof(size_t) + sizeof(unsigned int) * num_events;
  char response[response_len];
  size_t offset = 0;
  memset(response, 0, response_len);

  // [ result (int) ] | [ num_events (size_t) ] | [ events[num_events] (unsigned int) ]
  create_message(response, &offset, &result, sizeof(int));
  create_message(response, &offset, &num_events, sizeof(size_t));
  for (size_t i = 0; i < num_events; i++) {
    create_message(response, &offset, &events[i], sizeof(unsigned int));
  }

  // Connect to client pipe and send response
  int response_fd = open(client->response_pipename, O_WRONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  if (pipe_print(response_fd, &response, response_len)) {
    fprintf(stderr, "Failed to send response.\n");
    close(response_fd);
    return 1;
  }
  close(response_fd);

  free(events);
  return 0;
}