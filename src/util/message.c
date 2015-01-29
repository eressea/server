/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.

*/

#include <platform.h>
#include "message.h"

#include "goodies.h"
#include "log.h"
#include "quicklist.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void (*msg_log_create) (const struct message * msg) = 0;

const char *mt_name(const message_type * mtype)
{
  return mtype->name;
}

message_type *mt_new(const char *name, const char *args[])
{
  int i, nparameters = 0;
  message_type *mtype = (message_type *) malloc(sizeof(message_type));

  assert(name != NULL);
  if (name == NULL) {
    log_error("Trying to create message_type with name=0x0\n");
    return NULL;
  }
  if (args != NULL) {
    /* count the number of parameters */
    while (args[nparameters]) ++nparameters;
  }
  mtype->key = 0;
  mtype->name = _strdup(name);
  mtype->nparameters = nparameters;
  if (nparameters > 0) {
    mtype->pnames = (const char **)malloc(sizeof(char *) * nparameters);
    mtype->types = (const arg_type **)malloc(sizeof(arg_type *) * nparameters);
  } else {
    mtype->pnames = NULL;
    mtype->types = NULL;
  }
  if (args != NULL)
    for (i = 0; args[i]; ++i) {
      const char *x = args[i];
      const char *spos = strchr(x, ':');
      if (spos == NULL) {
        mtype->pnames[i] = _strdup(x);
        mtype->types[i] = NULL;
      } else {
        char *cp = strncpy((char *)malloc(spos - x + 1), x, spos - x);
        cp[spos - x] = '\0';
        mtype->pnames[i] = cp;
        mtype->types[i] = find_argtype(spos + 1);
        if (mtype->types[i] == NULL) {
          log_error("unknown argument type %s for message type %s\n", spos + 1, mtype->name);
        }
        assert(mtype->types[i]);
      }
    }
  return mtype;
}

message_type *mt_new_va(const char *name, ...)
{
  const char *args[16];
  int i = 0;
  va_list marker;

  va_start(marker, name);
  for (;;) {
    const char *c = va_arg(marker, const char *);
    args[i++] = c;
    if (c == NULL)
      break;
  }
  va_end(marker);
  return mt_new(name, args);
}

arg_type *argtypes = NULL;

void
register_argtype(const char *name, void (*free_arg) (variant),
  variant(*copy_arg) (variant), variant_type type)
{
  arg_type *atype = (arg_type *) malloc(sizeof(arg_type));
  atype->name = name;
  atype->next = argtypes;
  atype->release = free_arg;
  atype->copy = copy_arg;
  atype->vtype = type;
  argtypes = atype;
}

const arg_type *find_argtype(const char *name)
{
  arg_type *atype = argtypes;
  while (atype != NULL) {
    if (strcmp(atype->name, name) == 0)
      return atype;
    atype = atype->next;
  }
  return NULL;
}

static variant copy_arg(const arg_type * atype, variant data)
{
  assert(atype != NULL);
  if (atype->copy == NULL)
    return data;
  return atype->copy(data);
}

static void free_arg(const arg_type * atype, variant data)
{
  assert(atype != NULL);
  if (atype->release)
    atype->release(data);
}

message *msg_create(const struct message_type *mtype, variant args[])
{
  int i;
  message *msg = (message *) malloc(sizeof(message));

  assert(mtype != NULL);
  if (mtype == NULL) {
    log_error("Trying to create message with type=0x0\n");
    return NULL;
  }
  msg->type = mtype;
  msg->parameters = (variant *)(mtype->nparameters ? calloc(mtype->nparameters, sizeof(variant)) : NULL);
  msg->refcount = 1;
  for (i = 0; i != mtype->nparameters; ++i) {
    msg->parameters[i] = copy_arg(mtype->types[i], args[i]);
  }
  if (msg_log_create)
    msg_log_create(msg);
  return msg;
}

#define MT_MAXHASH 1021
static quicklist *messagetypes[MT_MAXHASH];

const message_type *mt_find(const char *name)
{
  unsigned int hash = hashstring(name) % MT_MAXHASH;
  quicklist *ql = messagetypes[hash];
  int qi;

  for (qi = 0; ql; ql_advance(&ql, &qi, 1)) {
    message_type *data = (message_type *) ql_get(ql, qi);
    if (strcmp(data->name, name) == 0) {
      return data;
    }
  }
  return 0;
}

static unsigned int mt_id(const message_type * mtype)
{
  unsigned int key = 0;
  size_t i = strlen(mtype->name);

  while (i > 0) {
    key = (mtype->name[--i] + key * 37);
  }
  return key % 0x7FFFFFFF;
}

const message_type *mt_register(message_type * type)
{
  unsigned int hash = hashstring(type->name) % MT_MAXHASH;
  quicklist **qlp = messagetypes + hash;

  if (ql_set_insert(qlp, type)) {
    type->key = mt_id(type);
  }
  return type;
}

void msg_free(message * msg)
{
  int i;
  assert(msg->refcount == 0);
  for (i = 0; i != msg->type->nparameters; ++i) {
    free_arg(msg->type->types[i], msg->parameters[i]);
  }
  free((void *)msg->parameters);
  free(msg);
}

void msg_release(struct message *msg)
{
  assert(msg->refcount > 0);
  if (--msg->refcount > 0)
    return;
  msg_free(msg);
}

struct message *msg_addref(struct message *msg)
{
  assert(msg->refcount > 0);
  ++msg->refcount;
  return msg;
}
