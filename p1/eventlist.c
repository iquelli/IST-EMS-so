#include "eventlist.h"
#include "utils.h"

#include <pthread.h>
#include <stdlib.h>

struct EventList *create_list() {
  struct EventList *list = (struct EventList *)malloc(sizeof(struct EventList));
  if (!list)
    return NULL;

  rwlock_init(&list->lock_list);

  list->head = NULL;
  list->tail = NULL;
  return list;
}

int append_to_list(struct EventList *list, struct Event *event) {
  rwlock_wrlock(&list->lock_list);
  if (!list) {
    rwlock_unlock(&list->lock_list);
    return 1;
  }

  struct ListNode *new_node =
      (struct ListNode *)malloc(sizeof(struct ListNode));
  if (!new_node) {
    rwlock_unlock(&list->lock_list);
    return 1;
  }

  new_node->event = event;
  new_node->next = NULL;

  if (list->head == NULL) {
    list->head = new_node;
    list->tail = new_node;
  } else {
    list->tail->next = new_node;
    list->tail = new_node;
  }

  rwlock_unlock(&list->lock_list);
  return 0;
}

static void free_event(struct Event *event) {
  if (!event)
    return;

  free(event->data);
  free(event);
}

void free_list(struct EventList *list) {
  if (!list)
    return;

  struct ListNode *current = list->head;
  while (current) {
    struct ListNode *temp = current;
    current = current->next;

    free_event(temp->event);
    free(temp);
  }

  free(list);
}

struct Event *get_event(struct EventList *list, unsigned int event_id) {
  rwlock_rdlock(&list->lock_list);
  if (!list) {
    rwlock_unlock(&list->lock_list);
    return NULL;
  }

  struct ListNode *current = list->head;
  while (current) {
    struct Event *event = current->event;
    if (event->id == event_id) {
      rwlock_unlock(&list->lock_list);
      return event;
    }
    current = current->next;
  }

  rwlock_unlock(&list->lock_list);
  return NULL;
}
