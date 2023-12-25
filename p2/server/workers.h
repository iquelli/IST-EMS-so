#ifndef __WORKERS_H__
#define __WORKERS_H__

/// Receives requests coming from the server pipe and stores it in the queue.
/// @param op_code Op code of the request to store.
/// @param server_pipe_fd File descripter of server pipe.
/// @return 0 if successfull, 1 otherwise.
int receive_request(int op_code, int server_pipe_fd);

/// Calls correct handling function for request.
/// @param arg Because of pthread create fuction.
void *process_incoming_request(void *arg);

// Handler functions for client requests.

// TODO: Add whatever params they need and comment function

int ems_setup_handler();

int ems_quit_handler();

int ems_create_handler();

int ems_reserve_handler();

int ems_show_handler();

int ems_list_handler();

#endif  // __WORKERS_H__