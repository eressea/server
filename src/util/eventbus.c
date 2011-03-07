#include <platform.h>
#include "eventbus.h"

/*
** first iteration. it is slow, and a proof of the concept - the simplest
** thing that would work, a.k.a. should be refectored when required.
*/

typedef struct listener {
  struct listener *next;
  event_handler callback;
  event_arg_free destroy;
  void *sender;
  char *event;
  void *arguments;
} listener;

static listener *listeners;

void eventbus_fire(void *sender, const char *event, void *args)
{
  listener *lst = listeners;

  while (lst) {
    int i = strcmp(lst->event, event);

    if (i > 0)
      break;
    if (i == 0) {
      if (!lst->sender || lst->sender == sender) {
        lst->callback(sender, event, args);
      }
    }
    lst = lst->next;
  }
}

void
eventbus_register(void *sender, const char *event, event_handler cb,
  event_arg_free arg_free, void *args)
{
  listener *lst;

  listener **lstp = &listeners;

  while (*lstp) {
    lst = *lstp;
    if (strcmp(lst->event, event) >= 0) {
      break;
    }
    lstp = &lst->next;
  }
  lst = malloc(sizeof(listener));
  lst->sender = sender;
  lst->arguments = args;
  lst->callback = cb;
  lst->destroy = arg_free;
  lst->event = strdup(event);
  lst->next = *lstp;
  *lstp = lst;
}
