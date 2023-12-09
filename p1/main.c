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
#include "utils.h"

#include "constants.h"
#include "operations.h"
#include "parser.h"

char *files[MAX_FILES];
int number_of_files = 0;
int MAX_THREADS;
int number_of_threads = 0;

pthread_mutex_t ems_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;
int barrier_counter = 0;

void *thread_function(void *arg);
int store_files(char *directory_path);
int file_handler(char *directory_path);

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc < 4) { // check if the input has at least 2 fields.
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
  int in_threads = atoi(argv[3]);
  MAX_THREADS = in_threads;

  if (pthread_barrier_init(&barrier, NULL,(unsigned int) MAX_THREADS) != 0) {
    fprintf(stderr, "Error initializing barrier\n");
    return 1;
  }

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
      exit(file_handler(files[i]) != 0);
    }

    // Control active processes
    while (active_processes == max_proc + 1) {
      int status;
      pid_t finished_pid = wait(&status);

      if (finished_pid > 0) {
        printf("Process %d finished\n", finished_pid);
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

  pthread_barrier_destroy(&barrier);

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
int file_handler(char *directory_path) {
  pthread_t threads[MAX_THREADS];

  while (1) {
    // Create a thread if the maximum number of threads is not reached
    if (number_of_threads < MAX_THREADS) {
      if (pthread_create(&threads[number_of_threads], NULL, thread_function, (void *)directory_path) != 0) {
        fprintf(stderr, "Error creating thread for file: %s\n", directory_path);
        exit(1);
      }
      number_of_threads++;
    }

    // Check for finished threads and join them
    for (int i = 0; i < number_of_threads; i++) {
      if (pthread_join(threads[i], NULL) != 0) {
        fprintf(stderr, "Error joining thread\n");
        exit(1);
      } else {
        // Thread finished, decrement the counter
        number_of_threads--;
      }
    }

    // Check for completion of all threads
    if (number_of_threads == 0) {
      break;
    }
  }

  return 0;
}

/*
  Thread function. Handles the commands from the file given.
*/
void *thread_function(void *arg) {
  char *directory_path = (char *)arg;
  int fd = open(directory_path, O_RDONLY);

  if (fd == -1) {
    fprintf(stderr, "Could not open file from the directory path %s\n",
            directory_path);
    pthread_exit(NULL);
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

    case CMD_BARRIER:
      pthread_mutex_lock(&ems_mutex);
      barrier_counter++;
      pthread_mutex_unlock(&ems_mutex);

      // Wait for all threads to reach the barrier
      pthread_barrier_wait(&barrier);

      // Only the last thread should reset the counter
      pthread_mutex_lock(&ems_mutex);
      if (barrier_counter == MAX_THREADS) {
        barrier_counter = 0;
      }
      pthread_mutex_unlock(&ems_mutex);
      break;

    case CMD_EMPTY:
      break;

    case EOC:
      ems_terminate();
      pthread_exit(NULL);
    }
  }

  close(fd);
  pthread_exit(NULL);
}