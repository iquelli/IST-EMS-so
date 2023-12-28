#include "workers.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Handlers

// TODO: these all should read the pipe and perform the intended action,
//       replying to client when necessary

int ems_setup_handler(int session_id, client_t *client) {
  // Open the client's response pipe for writing
  int response_fd = open(client->response_pipename, O_WRONLY);
  if (response_fd == -1) {
    fprintf(stderr, "Failed to open response pipe.\n");
    return 1;
  }

  // Send a response back to the client
  if (print_uint(response_fd, (unsigned)session_id)) {
    fprintf(stderr, "Failed to write to response pipe.\n");
    close(response_fd);
    return 1;
  }

  // Close the response pipe
  close(response_fd);

  return 0;
}

int ems_quit_handler(client_t *client);

int ems_create_handler(client_t *client);

int ems_reserve_handler(client_t *client);

int ems_show_handler(client_t *client);

int ems_list_handler(client_t *client);