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

char *files[MAX_FILES];
int number_of_files = 0;

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

  if (retrieve_job_files(argv[1], files, &number_of_files)) {
    fprintf(stderr, "Error retrieving files\n");
    return 1;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  int max_proc = atoi(argv[2]);
  int max_threads = atoi(argv[3]);

  // Iterate through files
  int active_processes = 0;
  for (int i = 0; i < number_of_files; i++) {

    active_processes++;
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr,
              "There was an error creating a process to handle file from "
              "directory path: %s\n",
              files[i]);
      exit(1);
    }
    if (pid == 0) {
      exit(process_job_file(files[i], max_threads) != 0);
    }

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

  // Free memory allocated
  for (int i = 0; i < number_of_files; i++) {
    free(files[i]);
  }

  return 0;
}