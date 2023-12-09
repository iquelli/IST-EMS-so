#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>

void rwlock_init(pthread_rwlock_t *lock);
void rwlock_destroy(pthread_rwlock_t *lock);
void rwlock_rdlock(pthread_rwlock_t *lock);
void rwlock_wrlock(pthread_rwlock_t *lock);
void rwlock_unlock(pthread_rwlock_t *lock);

void mutex_init(pthread_mutex_t *mutex);
void mutex_destroy(pthread_mutex_t *mutex);
void mutex_lock(pthread_mutex_t *mutex);
void mutex_unlock(pthread_mutex_t *mutex);

#endif // UTILS_H