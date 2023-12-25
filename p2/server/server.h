#ifndef __SERVER_H__
#define __SERVER_H__

#include <sys/types.h>

/// Initializes the server and EMS state.
/// @param pipename Name of the server pipe.
/// @param max_session Max number of clients the server will support.
/// @param delay Delay of EMS in microseconds.
/// @return 0 if the server was initialized successfully, 1 otherwise.
int server_init(char *pipename, size_t max_sessions, unsigned int delay_us);

/// Closes the server and EMS state.
/// @param pipename Name of the server pipe.
/// @param max_session Max number of clients the server will support.
/// @return 0 if the server was initialized successfully, 1 otherwise.
int server_close(char *pipename, size_t max_sessions);

/// Creates worker threads.
/// @param num_of_threads Number of threads to create.
/// @return 0 if the threads were created successfuly, 1 otherwise.
int workers_init(size_t num_of_threads);

#endif  // __SERVER_H__