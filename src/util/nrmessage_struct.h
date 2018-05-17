#ifndef CLASS_NRMESSAGE_STRUCT
#define CLASS_NRMESSAGE_STRUCT

/* This file should not be included by anything in the server. If you 
 * feel that you need to include it, it's a sure sign that you're trying to
 * do something BAD. */

typedef struct nrmessage_type {
  const struct message_type *mtype;
  char *vars;
  struct nrmessage_type *next;
  const char *section;
} nrmessage_type;

extern nrmessage_type *get_nrmessagetypes(void);

#endif
