#ifndef __PRODUCER_CONSUMER_H__
#define __PRODUCER_CONSUMER_H__

#include <pthread.h>

typedef struct {
  void **pcq_buffer;
  size_t pcq_capacity;

  pthread_mutex_t pcq_current_size_lock;
  size_t pcq_current_size;

  pthread_mutex_t pcq_head_lock;
  size_t pcq_head;

  pthread_mutex_t pcq_tail_lock;
  size_t pcq_tail;

  pthread_mutex_t pcq_pusher_condvar_lock;
  pthread_cond_t pcq_pusher_condvar;

  pthread_mutex_t pcq_popper_condvar_lock;
  pthread_cond_t pcq_popper_condvar;
} pc_queue_t;

/// Creates a queue with a fixed capacity.
/// @param queue Queue that will be created.
/// @param capacity Capacity of the queue.
/// @return 0 if successfull, 1 otherwise.
int pcq_create(pc_queue_t *queue, size_t capacity);

/// Destroys a queue.
/// @param queue Queue to be destroyed.
/// @return 0 if successfull, 1 otherwise.
int pcq_destroy(pc_queue_t *queue);

/// Adds a new element to the front of the queue.
/// @param queue Queue to be altered.
/// @param elem Element to add to the queue.
/// @return 0 if successfull, 1 otherwise.
int pcq_enqueue(pc_queue_t *queue, void *elem);

/// Removes an element from the back of the queue.
/// @param queue Queue to be altered.
/// @return 0 if successfull, 1 otherwise.
void *pcq_dequeue(pc_queue_t *queue);

#endif  // __PRODUCER_CONSUMER_H__