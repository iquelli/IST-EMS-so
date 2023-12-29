#include "io.h"

int parse_uint(int fd, unsigned int* value, char* next) {
  char buf[16];

  int i = 0;
  while (1) {
    ssize_t read_bytes = read(fd, buf + i, 1);
    if (read_bytes == -1) {
      return 1;
    } else if (read_bytes == 0) {
      *next = '\0';
      break;
    }

    *next = buf[i];

    if (buf[i] > '9' || buf[i] < '0') {
      buf[i] = '\0';
      break;
    }

    i++;
  }

  unsigned long ul = strtoul(buf, NULL, 10);

  if (ul > UINT_MAX) {
    return 1;
  }

  *value = (unsigned int)ul;

  return 0;
}

int print_uint(int fd, unsigned int value) {
  char buffer[16];
  size_t i = 16;

  for (; value > 0; value /= 10) {
    buffer[--i] = '0' + (char)(value % 10);
  }

  if (i == 16) {
    buffer[--i] = '0';
  }

  while (i < 16) {
    ssize_t written = write(fd, buffer + i, 16 - i);
    if (written == -1) {
      return 1;
    }

    i += (size_t)written;
  }

  return 0;
}

int print_str(int fd, const char* str) {
  size_t len = strlen(str);
  while (len > 0) {
    ssize_t written = write(fd, str, len);
    if (written == -1) {
      return 1;
    }

    str += (size_t)written;
    len -= (size_t)written;
  }

  return 0;
}

void create_message(void* message, size_t* offset, const void* data, size_t data_len) {
  memcpy(message + *offset, data, data_len);
  *offset += data_len;
}

int pipe_print(int pipe_fd, const void* buf, size_t buf_len) {
  size_t total_written = 0;

  while (total_written < buf_len) {
    ssize_t written = write(pipe_fd, buf + total_written, buf_len - total_written);

    if ((written == -1 && errno == EINTR) || (written == 0)) {
      return 1;  // Error other than interruption by signal or reached end of file
    }

    total_written += (size_t)written;
  }

  return 0;
}

int pipe_parse(int pipe_fd, void* buf, size_t buf_len) {
  size_t total_read = 0;

  while (total_read < buf_len) {
    ssize_t read_bytes = read(pipe_fd, (char*)buf + total_read, buf_len - total_read);

    if (read_bytes == -1 && errno != EINTR) {
      return 1;  // Error other than interruption
    }
    if (read_bytes == 0) {
      break;
    }

    total_read += (size_t)read_bytes;
  }

  if (total_read != buf_len) {
    return 1;  // Incomplete read
  }

  return 0;
}

int print_event(int out_fd, size_t num_rows, size_t num_cols, unsigned int* data) {
  for (size_t i = 1; i <= num_rows; i++) {
    for (size_t j = 1; j <= num_cols; j++) {
      char buffer[16];
      sprintf(buffer, "%u", data[(i - 1) * num_cols + j - 1]);
      if (print_str(out_fd, buffer)) {
        perror("Error writing to file descriptor");
        return 1;
      }

      if (j < num_cols) {
        if (print_str(out_fd, " ")) {
          perror("Error writing to file descriptor");
          return 1;
        }
      }
    }

    if (print_str(out_fd, "\n")) {
      perror("Error writing to file descriptor");
      return 1;
    }
  }

  return 0;
}

int print_ids(unsigned int* ids, size_t num_ids, int out_fd) {
  if (num_ids == 0) {
    char buff[] = "No events\n";
    if (print_str(out_fd, buff)) {
      perror("Error writing to file descriptor");
      return 1;
    }
    return 0;
  }

  for (size_t i = 0; i < num_ids; i++) {
    char buff[] = "Event: ";
    if (print_str(out_fd, buff)) {
      perror("Error writing to file descriptor");
      return 1;
    }

    char id[16];
    sprintf(id, "%u\n", ids[i]);
    if (print_str(out_fd, id)) {
      perror("Error writing to file descriptor");
      return 1;
    }
  }

  return 0;
}