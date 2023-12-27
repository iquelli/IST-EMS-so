#ifndef COMMON_IO_H
#define COMMON_IO_H

#include <stddef.h>

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

// TODO comment;
char *readString(const int fd, const size_t lim);

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

int read_token(int fd, char *message, size_t buf_len);

#endif  // COMMON_IO_H
