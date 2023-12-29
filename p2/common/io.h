#ifndef COMMON_IO_H
#define COMMON_IO_H

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "constants.h"

typedef struct {
  char request_pipename[CLIENT_PIPE_MAX_LEN];
  char response_pipename[CLIENT_PIPE_MAX_LEN];
  int session_id;
} client_t;

/// Parses an unsigned integer from the given file descriptor.
/// @param fd The file descriptor to read from.
/// @param value Pointer to the variable to store the value in.
/// @param next Pointer to the variable to store the next character in.
/// @return 0 if the integer was read successfully, 1 otherwise.
int parse_uint(int fd, unsigned int *value, char *next);

/// Prints an unsigned integer to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param value The value to write.
/// @return 0 if the integer was written successfully, 1 otherwise.
int print_uint(int fd, unsigned int value);

/// Writes a string to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param str The string to write.
/// @return 0 if the string was written successfully, 1 otherwise.
int print_str(int fd, const char *str);

/// Adds data to message to be sent to pipe. Used to preserve types.
/// @param message Message to be sent.
/// @param offset Offset to write from.
/// @param data Data to add.
/// @param data_len Length of data to add
void create_message(void *message, size_t *offset, const void *data, size_t data_len);

/// Writes a message in the pipe.
/// @param pipe_fd File descriptor to write in.
/// @param buf Content to write.
/// @param buf_len Length of the content to write
/// @return 0 if successful, 1 otherwise
int pipe_print(int pipe_fd, const void *buf, size_t buf_len);

/// Reads content from pipe of the a certain lenght and stores it.
/// @param pipe_fd File descriptor to read from.
/// @param buf Variable to store what is read.
/// @param buf_len Length to read.
/// @return 0 if successful, 1 otherwise.
int pipe_parse(int pipe_fd, void *buf, size_t buf_len);

#endif  // COMMON_IO_H
