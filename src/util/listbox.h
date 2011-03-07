#ifndef CURSES_LISTBOX
#define CURSES_LISTBOX

typedef struct list_selection {
  struct list_selection *next;
  struct list_selection *prev;
  int index;
  char *str;
  void *data;
} list_selection;

extern struct list_selection *do_selection(struct list_selection *sel,
  const char *title, void (*perform) (struct list_selection *, void *),
  void *data);
extern struct list_selection **push_selection(struct list_selection **p_sel,
  char *str, void *payload);
extern void insert_selection(struct list_selection **p_sel,
  struct list_selection *prev, const char *str, void *payload);

#endif /* CURSES_LISTBOX */
