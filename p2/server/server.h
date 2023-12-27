#ifndef __SERVER_H__
#define __SERVER_H__

#include <sys/types.h>

/// Initializes the server and EMS state.
/// @param pipename Name of the server pipe.
/// @param delay Delay of EMS in microseconds.
/// @return 0 if the server was initialized successfully, 1 otherwise.
int server_init(char *pipename, unsigned int delay_us);

/// Closes the server and EMS state.
/// @param pipename Name of the server pipe.
/// @param server_fd File descriptor of the server pipe.
/// @return 0 if the server was closed successfully, 1 otherwise.
int server_close(char *pipename, int server_fd);

/// Creates worker threads.
/// @return 0 if the threads were created successfuly, 1 otherwise.
int workers_init();

/// Adds a client to the queue
/// @param server_pipe_fd File descriptor of the server pipe.
/// @return 0 if successfull, 1 otherwise.
int receive_connection(int server_pipe_fd);

#endif  // __SERVER_H__