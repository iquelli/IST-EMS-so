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

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  // TODO: create pipes and connect to the server

  // Remove existing pipes and create new ones
  if ((unlink(req_pipe_path) != 0 && errno != ENOENT) || mkfifo(req_pipe_path, 0777) < 0) {
    fprintf(stderr, "Failed to create request pipe.\n");
    return 1;
  }

  if ((unlink(resp_pipe_path) != 0 && errno != ENOENT) || mkfifo(resp_pipe_path, 0777) < 0) {
    fprintf(stderr, "Failed to create response pipe.\n");
    return 1;
  }

  // Connect to server
  int server_fd = open(server_pipe_path, O_WRONLY);
  if (server_fd < 0) {
    fprintf(stderr, "Failed to open server pipe.\n");
    return 1;
  }

  unsigned int op_code = OP_CODE_SETUP_REQUEST;
  char request[CLIENT_PIPE_MAX_LEN];
  char response[CLIENT_PIPE_MAX_LEN];

  // Copy req_pipe_path to request
  strcpy(request, req_pipe_path);
  strcpy(response, resp_pipe_path);

  if (print_uint(server_fd, op_code) || print_str2(server_fd, request, CLIENT_PIPE_MAX_LEN * sizeof(char)) ||
      print_str2(server_fd, response, CLIENT_PIPE_MAX_LEN * sizeof(char))) {
    fprintf(stderr, "Could not write to server pipe.\n");
    close(server_fd);
    return 1;
  }

  close(server_fd);

  return 0;
}

int ems_quit(void) {
  // TODO: close pipes
  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  (void)event_id;
  (void)num_rows;
  (void)num_cols;
  // TODO: send create request to the server (through the request pipe) and wait for the response (through the response
  // pipe)
  return 1;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  (void)event_id;
  (void)num_seats;
  (void)xs;
  (void)ys;
  // TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response
  // pipe)
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  (void)out_fd;
  (void)event_id;
  // TODO: send show request to the server (through the request pipe) and wait for the response (through the response
  // pipe)
  return 1;
}

int ems_list_events(int out_fd) {
  (void)out_fd;
  // TODO: send list request to the server (through the request pipe) and wait for the response (through the response
  // pipe)
  return 1;
}
