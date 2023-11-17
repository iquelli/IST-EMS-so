#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// as que adicionei
#include <dirent.h>
#include <string.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc > 1) {
    char *endptr;
    unsigned long int delay = strtoul(argv[1], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (argc != 3) { // check if the input has 2 fields.
    fprintf(stderr, "Usage: %s <number> <directory_path>\n", argv[0]);
    return 1;
  }

  const char *directory_path = argv[2];
  char *files[MAX_FILES];
  DIR *dir;
  int number_of_files = 0;
  const char *file_extension_name = ".jobs";
  struct dirent *dir_entry;

  // Put .job files from the directory in the list files
  if ((dir = opendir(directory_path)) != NULL) {

    while ((dir_entry = readdir(dir)) != NULL && number_of_files < MAX_FILES) {

      if (strstr(dir_entry->d_name, file_extension_name) != NULL) {

        files[number_of_files] = malloc(MAX_DIRECTORY);
        snprintf(files[number_of_files], MAX_DIRECTORY, "%s/%s", directory_path,
                 dir_entry->d_name);
        number_of_files++;
      }
    }
    closedir(dir);
  } else {
    fprintf(stderr, "Could not open the directory\n");
  }

  if (number_of_files == 0) {
    fprintf(stderr, "No '.jobs' files in the directory\n");
    return 1;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  // Iterate through files
  for (int i = 0; i < number_of_files; i++) {

    int fd = open(files[i], O_RDONLY);

    if (fd == -1) {
      fprintf(stderr, "Could not open file from the directory path %s\n",
              files[i]);
      return 1;
    }

    int command;
    while ((command = get_next(fd)) != EOC) {
      unsigned int event_id, delay;
      size_t num_rows, num_columns, num_coords;
      size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

      switch (command) {
      case CMD_CREATE:
        if (parse_create(fd, &event_id, &num_rows, &num_columns) !=
            0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_create(event_id, num_rows, num_columns)) {
          fprintf(stderr, "Failed to create event\n");
        }

        break;

      case CMD_RESERVE:
        num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE,
                                   &event_id, xs, ys);

        if (num_coords == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_reserve(event_id, num_coords, xs, ys)) {
          fprintf(stderr, "Failed to reserve seats\n");
        }

        break;

      case CMD_SHOW:
        if (parse_show(fd, &event_id) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_show(event_id)) {
          fprintf(stderr, "Failed to show event\n");
        }

        break;

      case CMD_LIST_EVENTS:
        if (ems_list_events()) {
          fprintf(stderr, "Failed to list events\n");
        }

        break;

      case CMD_WAIT:
        if (parse_wait(fd, &delay, NULL) ==
            -1) { // thread_id is not implemented
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting...\n");
          ems_wait(delay);
        }

        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        printf("Available commands:\n"
               "  CREATE <event_id> <num_rows> <num_columns>\n"
               "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
               "  SHOW <event_id>\n"
               "  LIST\n"
               "  WAIT <delay_ms> [thread_id]\n" // thread_id is not implemented
               "  BARRIER\n"                     // Not implemented
               "  HELP\n");

        break;

      case CMD_BARRIER: // Not implemented
      case CMD_EMPTY:
        break;

      case EOC:
        ems_terminate();
        return 0;
      }
    }

    close(fd);
  }

  // Free memory allocated
  for (int i = 0; i < number_of_files; i++) {
    free(files[i]);
  }

  return 0;

  // TODO: Get directory of .jobs files and create a list
  //       if number of files = 0 print error "Failed to open directory or no
  //       .jobs files in directory"

  // TODO: For loop the files in the list

  // TODO: Inside the for loop, open the file in the beginning, do while
  // get_next(file_fd) != EOC
  //        to see when it finishes reading that file. When it finishes, close
  //        file.
  //       Keep rest of the code as is.
}