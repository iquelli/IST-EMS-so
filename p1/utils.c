#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void rwlock_init(pthread_rwlock_t *lock) {
  if (pthread_rwlock_init(lock, NULL) != 0) {
    perror("Failed to initalize the rwlock");
    exit(EXIT_FAILURE);
  }
}

void rwlock_destroy(pthread_rwlock_t *lock) {
  if (pthread_rwlock_destroy(lock) != 0) {
    perror("Failed to destroy the rwlock");
    exit(EXIT_FAILURE);
  }
}

void rwlock_rdlock(pthread_rwlock_t *lock) {
  if (pthread_rwlock_rdlock(lock) != 0) {
    perror("Failed to lock the read-only rwlock");
    exit(EXIT_FAILURE);
  }
}

void rwlock_wrlock(pthread_rwlock_t *lock) {
  if (pthread_rwlock_wrlock(lock) != 0) {
    perror("Failed to lock the write-read rwlock");
    exit(EXIT_FAILURE);
  }
}

void rwlock_unlock(pthread_rwlock_t *lock) {
  if (pthread_rwlock_unlock(lock) != 0) {
    perror("Failed to unlock the rwlock");
    exit(EXIT_FAILURE);
  }
}

void mutex_init(pthread_mutex_t *mutex) {
  if (pthread_mutex_init(mutex, NULL) != 0) {
    perror("Failed to Initialize the mutex");
    exit(EXIT_FAILURE);
  }
}

void mutex_destroy(pthread_mutex_t *mutex) {
  if (pthread_mutex_destroy(mutex) != 0) {
    perror("Failed to destroy the mutex");
    exit(EXIT_FAILURE);
  }
}

void mutex_lock(pthread_mutex_t *mutex) {
  if (pthread_mutex_lock(mutex) != 0) {
    perror("Failed to lock the mutex");
    exit(EXIT_FAILURE);
  }
}

void mutex_unlock(pthread_mutex_t *mutex) {
  if (pthread_mutex_unlock(mutex) != 0) {
    perror("Failed to unlock the mutex");
    exit(EXIT_FAILURE);
  }
}