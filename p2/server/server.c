#include "server.h"

#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

// Workers
// static pthread_t *worker; // TODO IMPLEMENT

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return EXIT_FAILURE;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  char *server_pipename = argv[1];

  // Intializes server, creates worker threads
  if (server_init(server_pipename, 1, state_access_delay_us)) {
    return EXIT_FAILURE;
  }

  while (1) {
    // TODO: Read from pipe
    // TODO: Write new client to the producer-consumer buffer
  }

  // TODO: Close Server

  ems_terminate();
  return 0;
}

int server_init(char *pipename, size_t max_sessions, unsigned int delay_us) {
  (void)pipename;
  (void)max_sessions;

  // Initialize EMS state.
  if (ems_init(delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return EXIT_FAILURE;
  }

  // TODO Creates producer-consumer queue. -- for later

  // TODO Mallocs space for worker threads
  // TODO Calls function to initialize the threads -- for now try out with one thread

  // TODO Actually boots up the server

  // IN CASE OF FAILURE DESTROY EMS SERVER, FREE CLIENTS, DESTROY QUEUES;

  return 0;
}

int server_close(char *pipename, size_t max_sessions) {
  // Make it do this every time in while(true) loop before doing exit failure
  // before it returns 0 in the end as well.

  (void)pipename;
  (void)max_sessions;

  if (ems_terminate()) {
    fprintf(stderr, "Failed to destroy EMS\n");
    return EXIT_FAILURE;
  }

  // TODO close and unlink pipe
  // TODO Destroy queue
  // TODO Free all memory allocated

  return 0;
}

int workers_init(size_t num_of_threads) {
  (void)num_of_threads;

  // TODO: Creates threads in for loop returns 1 in case fails to create one
  // Forwards them to the queue function -- todo later

  return 0;
}