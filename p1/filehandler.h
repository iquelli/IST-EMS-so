#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#define TRUE 1
#define FALSE 0

struct JobFile {
  int fd;
  int fd_out;
  int max_threads;
  pthread_t *threads;
  pthread_mutex_t file_mutex;
  unsigned int thread_id;
};

/*
  Retrieves files from the directory path given and stores them in files list.
  @return Returns 0 if successful, 1 otherwhise.
*/
int retrieve_job_files(char *directory_path, char *files[], int *num_of_files);

/*
  Opens .jobs and .out files from the directory path given.
  @return Returns 0 if successful, 1 otherwhise.
*/
int open_file(char *directory_path, struct JobFile *file);

/*
  Creates the threads for the file.
  @return Returns 0 if successful, 1 otherwhise.
*/
int create_threads(int max_threads, struct JobFile *file);

/*
  Process job files, create threads, and wait for their completion.
  @return Returns 0 if successful, 1 otherwise.
*/
int process_job_file(char *directory_path, int max_threads);

/*
    Executes file commands in a thread.
*/
void *execute_file_commands(void *thread);

#endif // FILE_HANDLER_H