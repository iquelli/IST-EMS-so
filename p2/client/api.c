#include "api.h"
#include "common/io.h"
#include "common/constants.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int fill_buffer(int fd, const char* str, size_t buf_len) {
  size_t len = strlen(str);

  // Preenche o restante do buffer com '\0' se necessário
  size_t padding_len = buf_len - len;
  if (padding_len > 0) {
    char padding[padding_len];
    memset(padding, '\0', padding_len);
    
    if (write(fd, padding, padding_len) == -1) {
      return 1;
    }
  }
}

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

  int fd = open(server_pipe_path, O_WRONLY);
  if (fd < 0) {
    fprintf(stderr, "Failed to open server pipe.\n");
    return 1;
  }

  unsigned int op_code = OP_CODE_SETUP_REQUEST;

  print_uint(fd, op_code);
  print_str(fd, req_pipe_path);
  fill_buffer(fd, req_pipe_path, CLIENT_PIPE_MAX_LEN);
  
  print_str(fd, resp_pipe_path);
  fill_buffer(fd, resp_pipe_path, CLIENT_PIPE_MAX_LEN);

  close(fd);

  return 0;
}

int ems_quit(void) {
  // TODO: close pipes
  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  // TODO: send create request to the server (through the request pipe) and wait for the response (through the response
  // pipe)
  return 1;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  // TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response
  // pipe)
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  // TODO: send show request to the server (through the request pipe) and wait for the response (through the response
  // pipe)
  return 1;
}

int ems_list_events(int out_fd) {
  // TODO: send list request to the server (through the request pipe) and wait for the response (through the response
  // pipe)
  return 1;
}
