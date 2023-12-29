#include "api.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/constants.h"
#include "common/io.h"

char client_req_pipe_path[CLIENT_PIPE_MAX_LEN] = {0};
char client_resp_pipe_path[CLIENT_PIPE_MAX_LEN] = {0};

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  // Remove existing pipes and create new ones
  if ((unlink(req_pipe_path) != 0 && errno != ENOENT) || mkfifo(req_pipe_path, 0777) < 0) {
    fprintf(stderr, "Failed to create request pipe.\n");
    return 1;
  }

  if ((unlink(resp_pipe_path) != 0 && errno != ENOENT) || mkfifo(resp_pipe_path, 0777) < 0) {
    fprintf(stderr, "Failed to create response pipe.\n");
    return 1;
  }

  // Initialize variables
  char op_code = OP_CODE_SETUP_REQUEST;
  strcpy(client_req_pipe_path, req_pipe_path);
  strcpy(client_resp_pipe_path, resp_pipe_path);

  size_t request_len = sizeof(char) + sizeof(char) * CLIENT_PIPE_MAX_LEN + sizeof(char) * CLIENT_PIPE_MAX_LEN;
  char request[request_len];
  size_t offset = 0;
  memset(request, 0, request_len);

  // Create message:
  // [ op_code (char) ] | [ client_request_pipe_path (char[40]) ] | [ client_response_pipe_path (char[40]) ]
  create_message(request, &offset, &op_code, sizeof(char));
  create_message(request, &offset, &client_req_pipe_path, CLIENT_PIPE_MAX_LEN * sizeof(char));
  create_message(request, &offset, &client_resp_pipe_path, CLIENT_PIPE_MAX_LEN * sizeof(char));

  // Connect to server and send request
  int server_fd = open(server_pipe_path, O_WRONLY);
  if (server_fd < 0) {
    fprintf(stderr, "Failed to open server pipe.\n");
    return 1;
  }
  if (pipe_print(server_fd, &request, request_len)) {
    fprintf(stderr, "Failed to send setup request to server pipe.\n");
    close(server_fd);
    return 1;
  }
  close(server_fd);

  // Receive response
  int response_fd = open(client_resp_pipe_path, O_RDONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  int session_id;
  if (pipe_parse(response_fd, &session_id, sizeof(int))) {
    fprintf(stderr, "Failed to read session id from server.\n");
    close(response_fd);
    return 1;
  }
  close(response_fd);

  printf("Setup completed successfully. Session ID %d has been assigned.\n", session_id);
  return 0;
}

int ems_quit(void) {
  char op_code = OP_CODE_QUIT_REQUEST;

  size_t request_len = sizeof(char);
  int8_t request[request_len];
  size_t offset = 0;
  memset(request, 0, request_len);

  // Create message:
  // [ op_code (char) ]
  create_message(request, &offset, &op_code, sizeof(char));

  // Open request pipe and send request.
  int client_req_fd = open(client_req_pipe_path, O_WRONLY);
  if (client_req_fd < 0) {
    fprintf(stderr, "Failed to open client request pipe.\n");
    return 1;
  }
  if (pipe_print(client_req_fd, &request, request_len)) {
    fprintf(stderr, "Failed to send setup request to server pipe.\n");
    close(client_req_fd);
    return 1;
  }
  close(client_req_fd);

  close(client_req_fd);
  unlink(client_req_pipe_path);
  unlink(client_resp_pipe_path);

  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  // Initialize variables
  char op_code = OP_CODE_CREATE_REQUEST;

  size_t request_len = sizeof(char) + sizeof(unsigned int) + sizeof(size_t) + sizeof(size_t);
  int8_t request[request_len];
  size_t offset = 0;
  memset(request, 0, request_len);

  // Create message:
  // [ op_code (char) ] | [ event_id (unsigned int) ] | [ num_rows (size_t) ] | [ num_cols (size_t)]
  create_message(request, &offset, &op_code, sizeof(char));
  create_message(request, &offset, &event_id, sizeof(unsigned int));
  create_message(request, &offset, &num_rows, sizeof(size_t));
  create_message(request, &offset, &num_cols, sizeof(size_t));

  // Open request pipe and send request.
  int client_req_fd = open(client_req_pipe_path, O_WRONLY);
  if (client_req_fd < 0) {
    fprintf(stderr, "Failed to open client request pipe.\n");
    return 1;
  }
  if (pipe_print(client_req_fd, &request, request_len)) {
    fprintf(stderr, "Failed to send setup request to server pipe.\n");
    close(client_req_fd);
    return 1;
  }
  close(client_req_fd);

  // Receive response
  int response_fd = open(client_resp_pipe_path, O_RDONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  int result;
  if (pipe_parse(response_fd, &result, sizeof(int))) {
    fprintf(stderr, "Failed to read result from server.\n");
    close(response_fd);
    return 1;
  }
  close(response_fd);

  printf("Event %s created.\n", result ? "failed to be" : "was");
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  // Initialize variables
  char op_code = OP_CODE_RESERVE_REQUEST;

  size_t request_len =
      sizeof(char) + sizeof(unsigned int) + sizeof(size_t) + sizeof(size_t) * num_seats + sizeof(size_t) * num_seats;

  int8_t request[request_len];
  size_t offset = 0;
  memset(request, 0, request_len);

  // Create message:
  // [ op_code (char) ] | [ event_id (unsigned int) ] | [ num_seats (size_t) ] | [ xs (size_t[num_seats]) ] | [ ys
  // (size_t[num_seats]) ]
  create_message(request, &offset, &op_code, sizeof(char));
  create_message(request, &offset, &event_id, sizeof(unsigned int));
  create_message(request, &offset, &num_seats, sizeof(size_t));
  for (size_t i = 0; i < num_seats; i++) {
    create_message(request, &offset, &xs[i], sizeof(size_t));
  }
  for (size_t i = 0; i < num_seats; i++) {
    create_message(request, &offset, &ys[i], sizeof(size_t));
  }

  // Open request pipe and send request.
  int client_req_fd = open(client_req_pipe_path, O_WRONLY);
  if (client_req_fd < 0) {
    fprintf(stderr, "Failed to open client request pipe.\n");
    return 1;
  }
  if (pipe_print(client_req_fd, &request, request_len)) {
    fprintf(stderr, "Failed to send setup request to server pipe.\n");
    close(client_req_fd);
    return 1;
  }
  close(client_req_fd);

  // Receive response
  int response_fd = open(client_resp_pipe_path, O_RDONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }
  int result;
  if (pipe_parse(response_fd, &result, sizeof(int))) {
    fprintf(stderr, "Failed to read result from server.\n");
    close(response_fd);
    return 1;
  }
  close(response_fd);

  printf("Event %s reserved.\n", result ? "failed to be" : "was");
  return 0;
}

int ems_show(int out_fd, unsigned int event_id) {
  // Initialize variables
  char op_code = OP_CODE_SHOW_REQUEST;

  size_t request_len = sizeof(char) + sizeof(unsigned int);
  int8_t request[request_len];
  size_t offset = 0;
  memset(request, 0, request_len);

  // Create message:
  // [ op_code (char) ] | [ event_id (unsigned int) ]
  create_message(request, &offset, &op_code, sizeof(char));
  create_message(request, &offset, &event_id, sizeof(unsigned int));

  // Open request pipe and send request.
  int client_req_fd = open(client_req_pipe_path, O_WRONLY);
  if (client_req_fd < 0) {
    fprintf(stderr, "Failed to open client request pipe.\n");
    return 1;
  }
  if (pipe_print(client_req_fd, &request, request_len)) {
    fprintf(stderr, "Failed to send setup request to server pipe.\n");
    close(client_req_fd);
    return 1;
  }
  close(client_req_fd);

  // Receive response
  int response_fd = open(client_resp_pipe_path, O_RDONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }

  int result;
  if (pipe_parse(response_fd, &result, sizeof(int))) {
    fprintf(stderr, "Failed to read result from server.\n");
    close(response_fd);
    return 1;
  }
  size_t num_rows;
  if (pipe_parse(response_fd, &num_rows, sizeof(size_t))) {
    fprintf(stderr, "Failed to read number of rows from server.\n");
    close(response_fd);
    return 1;
  }
  size_t num_cols;
  if (pipe_parse(response_fd, &num_cols, sizeof(size_t))) {
    fprintf(stderr, "Failed to read number of cols from server.\n");
    close(response_fd);
    return 1;
  }

  unsigned int* seats;
  seats = malloc(sizeof(unsigned int) * num_cols * num_rows);
  if (!seats) {
    perror("Memory allocation failed");
    free(seats);
    return 1;
  }
  for (size_t i = 0; i < num_cols * num_rows; i++) {
    if (pipe_parse(response_fd, &seats[i], sizeof(unsigned int))) {
      fprintf(stderr, "Failed to read seats from server.\n");
      free(seats);
      close(response_fd);
      return 1;
    }
  }

  if (result || print_event(out_fd, num_rows, num_cols, seats)) {
    return 1;
  }

  printf("Event %s shown.\n", result ? "failed to be" : "was");

  free(seats);
  close(response_fd);
  return 0;
}

int ems_list_events(int out_fd) {
  char op_code = OP_CODE_LIST_REQUEST;

  size_t request_len = sizeof(char);
  int8_t request[request_len];
  size_t offset = 0;
  memset(request, 0, request_len);

  // Create message:
  // [ op_code (char) ]
  create_message(request, &offset, &op_code, sizeof(char));

  // Open request pipe and send request.
  int client_req_fd = open(client_req_pipe_path, O_WRONLY);
  if (client_req_fd < 0) {
    fprintf(stderr, "Failed to open client request pipe.\n");
    return 1;
  }
  if (pipe_print(client_req_fd, &request, request_len)) {
    fprintf(stderr, "Failed to send setup request to server pipe.\n");
    close(client_req_fd);
    return 1;
  }
  close(client_req_fd);

  // Receive response
  int response_fd = open(client_resp_pipe_path, O_RDONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }

  int result;
  if (pipe_parse(response_fd, &result, sizeof(int))) {
    fprintf(stderr, "Failed to read result from server.\n");
    close(response_fd);
    return 1;
  }
  size_t num_events;
  if (pipe_parse(response_fd, &num_events, sizeof(size_t))) {
    fprintf(stderr, "Failed to read number of rows from server.\n");
    close(response_fd);
    return 1;
  }
  unsigned int* ids;
  ids = malloc(sizeof(unsigned int) * num_events);
  if (!ids) {
    perror("Memory allocation failed");
    free(ids);
    return 1;
  }

  for (size_t i = 0; i < num_events; i++) {
    if (pipe_parse(response_fd, &ids[i], sizeof(unsigned int))) {
      fprintf(stderr, "Failed to read ids from server.\n");
      free(ids);
      close(response_fd);
      return 1;
    }
  }

  if (result || print_ids(ids, num_events, out_fd)) {
    return 1;
  }

  printf("Event %s list.\n", result ? "failed to be" : "was");

  return 0;
}
