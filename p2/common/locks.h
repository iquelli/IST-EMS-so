#ifndef LOCKS_H
#define LOCKS_H

#include <pthread.h>

/// Initializes the rwlock. Exits if the initialization fails.
/// @param lock The rwlock to be initialized.
void rwlock_init(pthread_rwlock_t *lock);

/// Destroys the rwlock. Exits if the destruction fails.
/// @param lock The rwlock to be destroyed.
void rwlock_destroy(pthread_rwlock_t *lock);

/// Locks the rwlock for reading. Exits if the lock fails.
/// @param lock The rwlock to be locked.
void rwlock_rdlock(pthread_rwlock_t *lock);

/// Locks the rwlock for writing. Exits if the lock fails.
/// @param lock The rwlock to be locked.
void rwlock_wrlock(pthread_rwlock_t *lock);

/// Unlocks the rwlock. Exits if the unlock fails.
/// @param lock The rwlock to be unlocked.
void rwlock_unlock(pthread_rwlock_t *lock);

/// Initializes the mutex. Exits if the initialization fails.
/// @param mutex  The mutex to be initialized.
void mutex_init(pthread_mutex_t *mutex);

/// Destroys the mutex. Exits if the destruction fails.
/// @param mutex The mutex to be locked.
void mutex_destroy(pthread_mutex_t *mutex);

/// Locks the mutex. Exits if the lock fails.
/// @param mutex The mutex to be locked.
void mutex_lock(pthread_mutex_t *mutex);

/// Unlocks the mutex. Exits if the unlock fails.
/// @param mutex The mutex to be unlocked.
void mutex_unlock(pthread_mutex_t *mutex);

/// Initializes the cond variable. Exits if the initialization fails.
/// @param cond The cond variable to be initialized.
void cond_init(pthread_cond_t *cond);

/// Destroys thecond variable. Exits if the destruction fails.
/// @param cond  The cond variable to be destroys.
void cond_destroy(pthread_cond_t *cond);

/// Waits for the cond variable. Exits if the wait fails.
/// @param cond The cond variable to wait for.
void cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

/// Broadcasts a cond variable. Exits if the broadcast fails.
/// @param cond The cond variable to be broadcasted.
void cond_broadcast(pthread_cond_t *cond);

/// Signals a cond variable. Exits if the signal fails.
/// @param cond  The cond variable to be signalled.
void cond_signal(pthread_cond_t *cond);

#endif  // LOCKS_H