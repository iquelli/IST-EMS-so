#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

char *files[MAX_FILES];
int number_of_files = 0;
int number_of_threads = 0;

struct ThreadArgs {
  int fd;
  int fd_out;
  int max_threads;
  unsigned int thread_id;
};

void *thread_function(void *arg);
int store_files(char *directory_path);
int file_handler(char *directory_path, int max_threads);

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc < 4) { // check if the input has at least 3 fields.
    fprintf(stderr,
            "Usage: %s <directory_path> <max processes> <max threads> <(optional) delay>\n",
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

  if (store_files(argv[1]) == -1) {
    fprintf(stderr, "Error retrieving files\n");
    return 1;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  // Iterate through files
  int max_proc = atoi(argv[2]);
  int max_threads = atoi(argv[3]);

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
      exit(file_handler(files[i], max_threads) != 0);
    }

    // Control active processes
    while (active_processes == max_proc + 1) {
      int status;
      pid_t finished_pid = wait(&status);

      if (finished_pid > 0) {
        printf("Child process %d finished with state %d\n", finished_pid, WEXITSTATUS(status));
        active_processes--;
      }
    }
  }

  // Wait for all the child processes to complete
  while (active_processes > 0) {
    int status;
    pid_t finished_pid = wait(&status);

    if (finished_pid > 0) {
      printf("Child process %d finished with state %d\n", finished_pid, WEXITSTATUS(status));
      active_processes--;
    }
  }

  // Free memory allocated
  for (int i = 0; i < number_of_files; i++) {
    free(files[i]);
  }
  printf("All child processes have terminated\n");

  return 0;
}

/*
  Retrieves files from the directory path given and stores them. Returns 0 if
  successful, 1 otherwhise.
*/
int store_files(char *directory_path) {
  DIR *dir;
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

  return 0;
}

/*
  Handles the file given. Returns 0 if successful, 1 otherwhise.
*/
int file_handler(char *directory_path, int max_threads) {
  pthread_t threads[max_threads];
  struct ThreadArgs args[max_threads];

  int fd = open(directory_path, O_RDONLY);

  if (fd == -1) {
    fprintf(stderr, "Error opening file");
    return 1;
  }

  char *dot = strrchr(directory_path, '.');
  size_t length = (size_t)(dot - directory_path);

  char output_file_path[length + 5];
  strncpy(output_file_path, directory_path, length);
  strcpy(output_file_path + length, ".out");

  int fd_out = open(output_file_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

  if (fd_out == -1) {
    fprintf(stderr, "Error opening file");
    close(fd);
    return 1;
  }

  // Create threads
  for (int i = 0; i < max_threads; i++) {
    args[i].fd = fd;
    args[i].fd_out = fd_out;
    args[i].max_threads = max_threads;
    args[i].thread_id = (unsigned int) i;

    if (pthread_create(&threads[i], NULL, thread_function, (void *)&args[i]) != 0) {
      fprintf(stderr, "Error creating thread");
      close(fd_out);
      close(fd);
      return 1;
    }
  }

  // Join threads
  for (int i = 0; i < max_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error joining thread\n");
      close(fd_out);
      close(fd);
      return 1;
    }
  }

  if (close(fd_out) != 0) {
    fprintf(stderr, "Could not close the file\n");
    close(fd);
    return 1;
  }

  close(fd);
  return 0;
}

/*
  Thread function. Handles the commands from the file given.
*/
void *thread_function(void *arg) {
  struct ThreadArgs *args = (struct ThreadArgs *)arg;


  int command;
  while ((command = get_next(args->fd)) != EOC) {
    unsigned int event_id, delay, thread_id;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

    switch (command) {
    case CMD_CREATE:
      if (parse_create(args->fd, &event_id, &num_rows, &num_columns) != 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (ems_create(event_id, num_rows, num_columns)) {
        fprintf(stderr, "Failed to create event\n");
      }

      break;

    case CMD_RESERVE:
      num_coords = parse_reserve(args->fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);

      if (num_coords == 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (ems_reserve(event_id, num_coords, xs, ys)) {
        fprintf(stderr, "Failed to reserve seats\n");
      }
      break;

    case CMD_SHOW:
      if (parse_show(args->fd, &event_id) != 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }
      if (ems_show(event_id, args->fd_out)) {
        fprintf(stderr, "Failed to show event\n");
      }

      break;

    case CMD_LIST_EVENTS:
      if (ems_list_events(args->fd_out)) {
        fprintf(stderr, "Failed to list events\n");
      }

      break;

    case CMD_WAIT:
      if (parse_wait(args->fd, &delay, &thread_id) == -1) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (delay > 0 && (args->thread_id == thread_id || thread_id == 0)) {
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

    case CMD_BARRIER:
      break;

    case CMD_EMPTY:
      break;

    case EOC:
      ems_terminate();
      pthread_exit(NULL);
    }
  }
  pthread_exit(NULL);
}