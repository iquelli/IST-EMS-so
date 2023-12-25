#include "workers.h"

void *process_incoming_request(void *arg) {
  // TODO: get request from queue -- for later
  // TODO: read op code from request and forward it to correct handler
  // all op codes and in constants.h file
}

// Handlers

// TODO: these all should read the pipe and perform the intended action,
//       replying to client when necessary

int ems_setup_handler();

int ems_quit_handler();

int ems_create_handler();

int ems_reserve_handler();

int ems_show_handler();

int ems_list_handler();
