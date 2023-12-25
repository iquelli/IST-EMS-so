#include "eventlist.h"

#include <pthread.h>
#include <stdlib.h>

struct EventList* create_list() {
  struct EventList* list = (struct EventList*)malloc(sizeof(struct EventList));
  if (!list) return NULL;
  if (pthread_rwlock_init(&list->rwl, NULL) != 0) {
    free(list);
    return NULL;
  }
  list->head = NULL;
  list->tail = NULL;
  return list;
}

int append_to_list(struct EventList* list, struct Event* event) {
  if (!list) return 1;

  struct ListNode* new_node = (struct ListNode*)malloc(sizeof(struct ListNode));
  if (!new_node) return 1;

  new_node->event = event;
  new_node->next = NULL;

  if (list->head == NULL) {
    list->head = new_node;
    list->tail = new_node;
  } else {
    list->tail->next = new_node;
    list->tail = new_node;
  }

  return 0;
}

static void free_event(struct Event* event) {
  if (!event) return;
  free(event->data);
  free(event);
}

void free_list(struct EventList* list) {
  if (!list) return;

  struct ListNode* current = list->head;
  while (current) {
    struct ListNode* temp = current;
    current = current->next;

    free_event(temp->event);
    free(temp);
  }

  free(list);
}

struct Event* get_event(struct EventList* list, unsigned int event_id, struct ListNode* from, struct ListNode* to) {
  if (!list || !from || !to) return NULL;
  struct ListNode* current = from;

  while (1) {
    if (current->event->id == event_id) {
      return current->event;
    }

    if (current == to) {
      return NULL;
    }

    current = current->next;
  }
}
