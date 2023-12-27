#include "producer-consumer.h"

#include <stdlib.h>

#include "../common/locks.h"

int pcq_create(pc_queue_t *queue, size_t capacity) {
  if (queue == NULL || capacity <= 0) {
    return 1;
  }

  mutex_init(&queue->pcq_current_size_lock);
  mutex_lock(&queue->pcq_current_size_lock);
  queue->pcq_current_size = 0;
  mutex_unlock(&queue->pcq_current_size_lock);

  mutex_init(&queue->pcq_head_lock);
  mutex_lock(&queue->pcq_head_lock);
  queue->pcq_head = 0;
  mutex_unlock(&queue->pcq_head_lock);

  mutex_init(&queue->pcq_tail_lock);
  mutex_lock(&queue->pcq_tail_lock);
  queue->pcq_tail = 0;
  mutex_unlock(&queue->pcq_tail_lock);

  mutex_init(&queue->pcq_pusher_condvar_lock);
  cond_init(&queue->pcq_pusher_condvar);

  mutex_init(&queue->pcq_popper_condvar_lock);
  cond_init(&queue->pcq_popper_condvar);

  queue->pcq_capacity = capacity;
  queue->pcq_buffer = (void **)malloc(capacity * sizeof(void *));

  return 0;
}

int pcq_destroy(pc_queue_t *queue) {
  if (queue == NULL || queue->pcq_buffer == NULL) {
    return 1;
  }

  free(queue->pcq_buffer);
  queue->pcq_buffer = NULL;

  mutex_destroy(&queue->pcq_current_size_lock);

  mutex_destroy(&queue->pcq_head_lock);

  mutex_destroy(&queue->pcq_tail_lock);

  mutex_destroy(&queue->pcq_pusher_condvar_lock);
  cond_destroy(&queue->pcq_pusher_condvar);

  mutex_destroy(&queue->pcq_popper_condvar_lock);
  cond_destroy(&queue->pcq_popper_condvar);

  return 0;
}

int pcq_enqueue(pc_queue_t *queue, void *elem) {
  if (queue == NULL || queue->pcq_buffer == NULL) {
    return 1;
  }

  // We wait until we have an empty slot to enqueue the element
  mutex_lock(&queue->pcq_pusher_condvar_lock);
  mutex_lock(&queue->pcq_current_size_lock);
  while (queue->pcq_current_size == queue->pcq_capacity) {
    mutex_unlock(&queue->pcq_current_size_lock);
    cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_pusher_condvar_lock);
    mutex_lock(&queue->pcq_current_size_lock);
  }

  mutex_lock(&queue->pcq_tail_lock);
  queue->pcq_buffer[queue->pcq_tail] = elem;
  queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;
  mutex_unlock(&queue->pcq_tail_lock);

  queue->pcq_current_size++;
  mutex_unlock(&queue->pcq_current_size_lock);
  mutex_unlock(&queue->pcq_pusher_condvar_lock);

  mutex_lock(&queue->pcq_popper_condvar_lock);
  cond_signal(&queue->pcq_popper_condvar);
  mutex_unlock(&queue->pcq_popper_condvar_lock);

  return 0;
}

void *pcq_dequeue(pc_queue_t *queue) {
  if (queue == NULL || queue->pcq_buffer == NULL) {
    return NULL;
  }

  // We wait until we have an element to dequeue
  mutex_lock(&queue->pcq_popper_condvar_lock);
  mutex_lock(&queue->pcq_current_size_lock);
  while (queue->pcq_current_size == 0) {
    mutex_unlock(&queue->pcq_current_size_lock);
    cond_wait(&queue->pcq_popper_condvar, &queue->pcq_popper_condvar_lock);
    mutex_lock(&queue->pcq_current_size_lock);
  }

  mutex_lock(&queue->pcq_head_lock);
  void *elem = queue->pcq_buffer[queue->pcq_head];
  queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;
  mutex_unlock(&queue->pcq_head_lock);

  queue->pcq_current_size--;
  mutex_unlock(&queue->pcq_current_size_lock);
  mutex_unlock(&queue->pcq_popper_condvar_lock);

  mutex_lock(&queue->pcq_pusher_condvar_lock);
  cond_signal(&queue->pcq_pusher_condvar);
  mutex_unlock(&queue->pcq_pusher_condvar_lock);

  return elem;
}