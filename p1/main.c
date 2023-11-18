#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// as que adicionei
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

int file_handler(char *directory_path);

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc < 3) { // check if the input has at least 2 fields.
    fprintf(stderr,
            "Usage: %s <directory_path> <max processes> <(optional) delay>\n",
            argv[0]);
    return 1;
  }

  if (argc > 3) { // in case a delay was specified
    char *endptr;
    unsigned long int delay = strtoul(argv[3], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  const char *directory_path = argv[1];
  int max_proc = atoi(argv[2]);

  char *files[MAX_FILES];

  DIR *dir;
  int number_of_files = 0;
  struct dirent *dir_entry;

  if ((dir = opendir(directory_path)) == NULL) {
    fprintf(stderr, "Could not open the directory\n");
    return 1;
  }

  // Put .job files from the directory in the list files
  while ((dir_entry = readdir(dir)) != NULL && number_of_files < MAX_FILES) {
    if (strstr(dir_entry->d_name, ".jobs") != NULL) {
      files[number_of_files] = malloc(MAX_DIRECTORY);
      snprintf(files[number_of_files], MAX_DIRECTORY, "%s/%s", directory_path,
               dir_entry->d_name);
      number_of_files++;
    }
  }

  closedir(dir);

  if (number_of_files == 0) {
    fprintf(stderr, "No '.jobs' files in the directory\n");
    return 1;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  // Iterate through files
  int active_processes = 0;
  for (int i = 0; i < number_of_files; i++) {

    active_processes++;
    pid_t pid = fork();

    if (pid < 0) {
      fprintf(stderr,
              "There was an error creating a process to handle file from "
              "directory path: %s\n",
              directory_path);
      exit(1);
    }
    if (pid == 0) {
      exit(file_handler(files[i]) != 0);
    }

    // Control active processes
    while (active_processes == max_proc + 1) {
      int status;
      pid_t finished_pid = wait(&status);

      if (finished_pid > 0) {
        printf("Process %d has finish.\n", finished_pid);
        active_processes--;
      }
    }
  }

  // Wait for all the child processes to complete
  while (active_processes > 0) {
      int status;
      pid_t finished_pid = wait(&status);

      if (finished_pid > 0) {
        printf("Process %d finished\n", finished_pid);
        active_processes--;
      }
  }

  // Free memory allocated
  for (int i = 0; i < number_of_files; i++) {
    free(files[i]);
  }

  printf("All child processes have terminated.\n");

  return 0;
}

/*
  Processes '.jobs' files. Returns 0 if successful, 1 otherwhise.
*/
int file_handler(char *directory_path) {

  int fd = open(directory_path, O_RDONLY);

  if (fd == -1) {
    fprintf(stderr, "Could not open file from the directory path %s\n",
            directory_path);
    return 1;
  }

  int command;
  while ((command = get_next(fd)) != EOC) {
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

    switch (command) {
    case CMD_CREATE:
      if (parse_create(fd, &event_id, &num_rows, &num_columns) != 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (ems_create(event_id, num_rows, num_columns)) {
        fprintf(stderr, "Failed to create event\n");
      }

      break;

    case CMD_RESERVE:
      num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);

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

      if (ems_show(event_id, directory_path)) {
        fprintf(stderr, "Failed to show event\n");
      }

      break;

    case CMD_LIST_EVENTS:
      if (ems_list_events()) {
        fprintf(stderr, "Failed to list events\n");
      }

      break;

    case CMD_WAIT:
      if (parse_wait(fd, &delay, NULL) == -1) { // thread_id is not implemented
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

  return 0;
}