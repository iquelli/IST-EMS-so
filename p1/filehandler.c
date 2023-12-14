#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
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
#include "utils.h"

pthread_mutex_t file_mutex;
int BARRIER = TRUE;

int retrieve_job_files(char *directory_path, char *files[], int *num_of_files) {
  DIR *dir;
  struct dirent *dir_entry;

  if ((dir = opendir(directory_path)) == NULL) {
    fprintf(stderr, "Could not open the directory\n");
    return 1;
  }

  // Put .job files from the directory in the list files
  while ((dir_entry = readdir(dir)) != NULL && (*num_of_files) < MAX_FILES) {
    if (strstr(dir_entry->d_name, ".jobs") != NULL) {
      files[(*num_of_files)] = (char *)malloc(MAX_DIRECTORY);
      if (files[(*num_of_files)] == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        closedir(dir);
        return 1;
      }
      snprintf(files[(*num_of_files)], MAX_DIRECTORY, "%s/%s", directory_path,
               dir_entry->d_name);
      (*num_of_files)++;
    }
  }

  closedir(dir);

  if (num_of_files == 0) {
    fprintf(stderr, "No '.jobs' files in the directory\n");
    return 1;
  }

  return 0;
}

int open_file(char *directory_path, struct JobFile *file) {
  file->fd = open(directory_path, O_RDONLY);
  if (file->fd == -1) {
    perror("Error opening input file");
    return 1;
  }

  char *dot = strrchr(directory_path, '.');
  size_t length = (size_t)(dot - directory_path);

  char output_file_path[length + 5];
  strncpy(output_file_path, directory_path, length);
  strcpy(output_file_path + length, ".out");

  file->fd_out =
      open(output_file_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (file->fd_out == -1) {
    perror("Error opening output file");
    close(file->fd);
    return 1;
  }

  return 0;
}

int process_job_file(char *directory_path, int max_threads) {
  struct JobFile *file = malloc(sizeof(struct JobFile));

  if (open_file(directory_path, file) != 0) {
    mutex_destroy(&file->file_mutex);
    return 1;
  }

  file->max_threads = max_threads;
  file->threads = malloc(sizeof(pthread_t) * (size_t)max_threads);

  if (file->threads == NULL) {
    perror("Error allocating memory for threads");
    close(file->fd_out);
    close(file->fd);
    return 1;
  }

  mutex_init(&file->file_mutex);

  while (BARRIER) {
    BARRIER = FALSE; // reset barrier

    // Create threads
  for (int i = 0; i < max_threads; i++) {
    struct ThreadArgs *thread_args = malloc(sizeof(struct ThreadArgs));
    if (thread_args == NULL) {
      perror("Error allocating memory for thread arguments");
      mutex_destroy(&file->file_mutex);
      close(file->fd_out);
      close(file->fd);
      return 1;
    }
    thread_args->file = malloc(sizeof(struct JobFile));
    if (thread_args->file == NULL) {
      perror("Error allocating memory for JobFile structure");
      free(thread_args);
      exit(EXIT_FAILURE);
    }
    thread_args->file = file;
    thread_args->thread_id = (unsigned int)i;

    if (pthread_create(&file->threads[i], NULL, execute_file_commands,
                       (void *)thread_args) != 0) {
      perror("Error creating thread");
      free(thread_args);
      mutex_destroy(&file->file_mutex);
      close(file->fd_out);
      close(file->fd);
      return 1;
    }
  }

  // Join threads
  for (int i = 0; i < max_threads; i++) {
    if (pthread_join(file->threads[i], NULL) != 0) {
      perror("Error joining thread");
    }
  }
  }

  if (close(file->fd_out) != 0) {
    perror("Could not close the output file");
  }

  if (close(file->fd) != 0) {
    perror("Could not close the input file");
  }

  mutex_destroy(&file->file_mutex);

  return 0;
}

void *execute_file_commands(void *thread) {
  struct ThreadArgs *thread_args = (struct ThreadArgs *)thread;

  mutex_lock(&thread_args->file->file_mutex);
  int command = get_next(thread_args->file->fd);
  while (command != EOC) {

    unsigned int event_id, delay, thread_id;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    int result;

    switch (command) {
    case CMD_CREATE:
      result = parse_create(thread_args->file->fd, &event_id, &num_rows,
                            &num_columns);
      if (result != 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        mutex_unlock(&thread_args->file->file_mutex);
        continue;
      }

      if (ems_create(event_id, num_rows, num_columns)) {
        fprintf(stderr, "Failed to create event\n");
      }

      mutex_unlock(&thread_args->file->file_mutex);

      break;

    case CMD_RESERVE:
      num_coords = parse_reserve(thread_args->file->fd, MAX_RESERVATION_SIZE,
                                 &event_id, xs, ys);
      mutex_unlock(&thread_args->file->file_mutex);

      if (num_coords == 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (ems_reserve(event_id, num_coords, xs, ys)) {
        fprintf(stderr, "Failed to reserve seats\n");
      }

      break;

    case CMD_SHOW:
      result = parse_show(thread_args->file->fd, &event_id);
      mutex_unlock(&thread_args->file->file_mutex);
      if (result != 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }
      if (ems_show(event_id, thread_args->file->fd_out)) {
        fprintf(stderr, "Failed to show event\n");
      }

      break;

    case CMD_LIST_EVENTS:
      mutex_unlock(&thread_args->file->file_mutex);
      if (ems_list_events(thread_args->file->fd_out)) {
        fprintf(stderr, "Failed to list events\n");
      }
      break;

    case CMD_WAIT:
      result = parse_wait(thread_args->file->fd, &delay, &thread_id);
      mutex_unlock(&thread_args->file->file_mutex);

      if (result == -1) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (delay > 0 &&
          (thread_args->thread_id == thread_id || thread_id == 0)) {
        printf("Waiting...\n");
        ems_wait(delay);
      }

      break;

    case CMD_INVALID:
      mutex_unlock(&thread_args->file->file_mutex);
      fprintf(stderr, "Invalid command. See HELP for usage\n");
      break;

    case CMD_HELP:
      mutex_unlock(&thread_args->file->file_mutex);
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
      BARRIER = TRUE;
      mutex_unlock(&thread_args->file->file_mutex);
      pthread_exit(NULL);

    case CMD_EMPTY:
      mutex_unlock(&thread_args->file->file_mutex);
      break;

    case EOC:
      ems_terminate();
      mutex_unlock(&thread_args->file->file_mutex);
      pthread_exit(NULL);
    }

    // lock before getting next command
    mutex_lock(&thread_args->file->file_mutex);
    command = get_next(thread_args->file->fd);
  }

  mutex_unlock(&thread_args->file->file_mutex);
  pthread_exit(NULL);
}