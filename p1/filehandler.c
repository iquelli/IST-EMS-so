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

int BARRIER = FALSE;
int WAIT = FALSE;
unsigned int *wait_time;

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

int create_threads(int max_threads, struct JobFile *file) {
  for (int i = 0; i < max_threads; i++) {
    mutex_lock(&file->file_mutex);
    file->thread_id = (unsigned int)i + 1;
    mutex_unlock(&file->file_mutex);

    if (pthread_create(&file->threads[i], NULL, execute_file_commands,
                       (void *)file) != 0) {
      perror("Error creating thread");
      mutex_destroy(&file->file_mutex);
      close(file->fd_out);
      close(file->fd);
      return 1;
    }
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
  wait_time = malloc(sizeof(unsigned int) * (size_t)max_threads);

  if (file->threads == NULL) {
    perror("Error allocating memory for threads");
    close(file->fd_out);
    close(file->fd);
    return 1;
  }

  mutex_init(&file->file_mutex);

  if (create_threads(max_threads, file) != 0) {
    perror("Could not execute program.");
    return 1;
  }

  int exit = FALSE;
  while (TRUE) {
    void *result = malloc(sizeof(int));
    for (int i = 0; i < max_threads; i++) {
      if (pthread_join(file->threads[i], &result) != 0) {
        perror("Error joining thread");
      }
      int thread_result = *(int *)result;
      if (thread_result == 1) { // left because of the end of the file
        exit = TRUE;
      }
    }
    free(result);

    if (exit) {
      break;
    }

    // continues because the file hasnt ended, and left because of a barrier
    BARRIER = FALSE;
    create_threads(max_threads, file);
  }

  if (close(file->fd_out) != 0) {
    perror("Could not close the output file");
  }

  if (close(file->fd) != 0) {
    perror("Could not close the input file");
  }

  mutex_destroy(&file->file_mutex);
  free(file->threads);
  free(file);

  return 0;
}

void *execute_file_commands(void *file) {
  struct JobFile *thread_args = (struct JobFile *)file;

  mutex_lock(&thread_args->file_mutex);
  unsigned int thread_id = thread_args->thread_id;
  int command = get_next(thread_args->fd);
  int threads_that_need_wait;
  int *thread_result = malloc(sizeof(int));

  while (command != EOC) {
    unsigned int event_id, delay, thread_id_wait;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    int result;

    switch (command) {
    case CMD_CREATE:
      result =
          parse_create(thread_args->fd, &event_id, &num_rows, &num_columns);
      mutex_unlock(&thread_args->file_mutex);

      if (result != 0) {
        fprintf(stderr, "Invalid create command. See HELP for usage\n");
        continue;
      }

      if (ems_create(event_id, num_rows, num_columns)) {
        fprintf(stderr, "Failed to create event\n");
      }

      break;

    case CMD_RESERVE:
      num_coords = parse_reserve(thread_args->fd, MAX_RESERVATION_SIZE,
                                 &event_id, xs, ys);
      mutex_unlock(&thread_args->file_mutex);

      if (num_coords == 0) {
        fprintf(stderr, "Invalid reserve command. See HELP for usage\n");
        continue;
      }

      if (ems_reserve(event_id, num_coords, xs, ys)) {
        fprintf(stderr, "Failed to reserve seats\n");
      }

      // mutex_unlock(&thread_args->file->file_mutex);
      break;

    case CMD_SHOW:
      result = parse_show(thread_args->fd, &event_id);
      mutex_unlock(&thread_args->file_mutex);
      if (result != 0) {
        fprintf(stderr, "Invalid show command. See HELP for usage\n");
        continue;
      }
      if (ems_show(event_id, thread_args->fd_out)) {
        fprintf(stderr, "Failed to show event\n");
      }

      break;

    case CMD_LIST_EVENTS:
      mutex_unlock(&thread_args->file_mutex);
      if (ems_list_events(thread_args->fd_out)) {
        fprintf(stderr, "Failed to list events\n");
      }
      break;

    case CMD_WAIT:
      result = parse_wait(thread_args->fd, &delay, &thread_id_wait);
      mutex_unlock(&thread_args->file_mutex);

      if (result == -1) {
        fprintf(stderr, "Invalid wait command. See HELP for usage\n");
        continue;
      }
      if (delay == 0) {
        continue;
      }

      mutex_lock(&thread_args->file_mutex);
      WAIT = TRUE;
      if (thread_id_wait == 0) {
        for (int i = 0; i < thread_args->max_threads; i++) {
          wait_time[i] = delay;
        }
        threads_that_need_wait = thread_args->max_threads;
      } else if ((int)thread_id_wait < thread_args->max_threads) {
        wait_time[thread_id_wait - 1] = delay;
        threads_that_need_wait = 1;
      }
      mutex_unlock(&thread_args->file_mutex);

      break;

    case CMD_INVALID:
      mutex_unlock(&thread_args->file_mutex);
      fprintf(stderr, "Invalid command. See HELP for usage\n");
      break;

    case CMD_HELP:
      mutex_unlock(&thread_args->file_mutex);
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
      BARRIER = TRUE; // Set termination cause to BARRIER
      mutex_unlock(&thread_args->file_mutex);
      *thread_result = 0;
      pthread_exit(thread_result);

    case CMD_EMPTY:
      mutex_unlock(&thread_args->file_mutex);
      break;

    case EOC:
      mutex_unlock(&thread_args->file_mutex);

      *thread_result = 0;
      if (command == EOC) {
        *thread_result = 1;
      }
      pthread_exit(thread_result);
    }

    // lock before getting next command
    mutex_lock(&thread_args->file_mutex);

    // check if there needs to be wait for the thread
    if (WAIT == TRUE) {
      if ((wait_time[thread_id - 1] > 0)) {
        printf("Waiting...\n");
        ems_wait(wait_time[thread_id - 1]);
        wait_time[thread_id - 1] = 0;
        threads_that_need_wait -= 1;
      }
      if (threads_that_need_wait == 0) {
        WAIT = FALSE;
      }
    }

    if (BARRIER) {
      break;
    }

    command = get_next(thread_args->fd);
  }

  mutex_unlock(&thread_args->file_mutex);

  *thread_result = 0;
  if (command == EOC) {
    *thread_result = 1;
  }
  pthread_exit(thread_result);
}