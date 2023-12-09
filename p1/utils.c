#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Initializes a rwlock. When unsuccessful, it exits with failure.
 */
void rwlock_init(pthread_rwlock_t *lock) {
    if (pthread_rwlock_init(lock, NULL) != 0) {
        perror("Failed to initalize the rwlock");
        exit(EXIT_FAILURE);
    }
}

/**
 * Destroys a rwlock. When unsuccessful, it exits with failure.
 */
void rwlock_destroy(pthread_rwlock_t *lock) {
    if (pthread_rwlock_destroy(lock) != 0) {
        perror("Failed to destroy the rwlock");
        exit(EXIT_FAILURE);
    }
}

/**
 * Locks a rwlock to read-only. When unsuccessful, it exits with failure.
 */
void rwlock_rdlock(pthread_rwlock_t *lock) {
    if (pthread_rwlock_rdlock(lock) != 0) {
        perror("Failed to lock the rwlock to read-only");
        exit(EXIT_FAILURE);
    }
}

/**
 * Locks a rwlock to write-read. When unsuccessful, it exits with failure.
 */
void rwlock_wrlock(pthread_rwlock_t *lock) {
    if (pthread_rwlock_wrlock(lock) != 0) {
        perror("Failed to lock the rwlock to write-read");
        exit(EXIT_FAILURE);
    }
}

/**
 * Unlocks a rwlock. When unsuccessful, it exits with failure.
 */
void rwlock_unlock(pthread_rwlock_t *lock) {
    if (pthread_rwlock_unlock(lock) != 0) {
        perror("Failed to unlock the rwlock");
        exit(EXIT_FAILURE);
    }
}

/**
 * Initializes a mutex. When unsuccessful, it exits with failure.
 */
void mutex_init(pthread_mutex_t *mutex) {
    if (pthread_mutex_init(mutex, NULL) != 0) {
        perror("Failed to Initialize the mutex");
        exit(EXIT_FAILURE);
    }
}

/**
 * Destroys a mutex. When unsuccessful, it exits with failure.
 */
void mutex_destroy(pthread_mutex_t *mutex) {
    if (pthread_mutex_destroy(mutex) != 0) {
        perror("Failed to destroy the mutex");
        exit(EXIT_FAILURE);
    }
}

/**
 * Locks a mutex. When unsuccessful, it exits with failure.
 */
void mutex_lock(pthread_mutex_t *mutex) {
    if (pthread_mutex_lock(mutex) != 0) {
        perror("Failed to lock the mutex");
        exit(EXIT_FAILURE);
    }
}

/**
 * Unlocks a mutex. When unsuccessful, it exits with failure.
 */
void mutex_unlock(pthread_mutex_t *mutex) {
    if (pthread_mutex_unlock(mutex) != 0) {
        perror("Failed to unlock the mutex");
        exit(EXIT_FAILURE);
    }
}