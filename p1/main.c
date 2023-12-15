#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "constants.h"
#include "filehandler.h"
#include "operations.h"
#include "parser.h"

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc < 4) { // check if the input has at least 3 fields.
    fprintf(stderr,
            "Usage: %s <directory_path> <max processes> <max threads> "
            "<(optional) delay>\n",
            argv[0]);
    return 1;
  }

  if (argc > 4) { // in case a delay was specified
    char *endptr;
    unsigned long int delay = strtoul(argv[4], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  char *directory_path = argv[1];
  int max_proc = atoi(argv[2]);
  int max_threads = atoi(argv[3]);

  DIR *dir;
  struct dirent *dir_entry;

  if ((dir = opendir(directory_path)) == NULL) {
    fprintf(stderr, "Could not open the directory\n");
    return 1;
  }

  // Iterate through files
  int active_processes = 0;
  while((dir_entry = readdir(dir)) != NULL ) {
    char* filename;
    if (strstr(dir_entry->d_name, ".jobs") != NULL) {
      size_t filename_size = sizeof(directory_path) +  sizeof(dir_entry->d_name) + 2;
      filename = malloc(filename_size);
      snprintf(filename, filename_size, "%s/%s", directory_path,
               dir_entry->d_name);
    } else {
      continue;
    }

    active_processes++;
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr,
              "There was an error creating a process to handle file from "
              "directory path: %s\n",
              filename);
      exit(1);
    }
    if (pid == 0) {
      exit(process_job_file(filename, max_threads) != 0);
     exit(0);
    }

    free(filename);

    // Control active processes
    while (active_processes == max_proc + 1) {
      int status;
      pid_t finished_pid = wait(&status);

      if (finished_pid > 0) {
        printf("Child process %d finished with state %d\n", finished_pid,
               WEXITSTATUS(status));
        active_processes--;
      }
    }
  }

  // Wait for all the child processes to complete
  while (active_processes > 0) {
    int status;
    pid_t finished_pid = wait(&status);

    if (finished_pid > 0) {
      printf("Child process %d finished with state %d\n", finished_pid,
             WEXITSTATUS(status));
      active_processes--;
    }
  }
  printf("All child processes have terminated\n");

  return 0;
}