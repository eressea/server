/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.

*/

#include <config.h>
#include "message.h"

#include "goodies.h"
#include "log.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

const char *
mt_name(const message_type* mtype)
{
	return mtype->name;
}

message_type *
mt_new(const char * name, const char * args[])
{
  int i, nparameters = 0;
  message_type * mtype = (message_type *)malloc(sizeof(message_type));

  assert(name!=NULL);
  if (name==NULL) {
    log_error(("Trying to create message_type with name=0x0\n"));
    return NULL;
  }
  if (args!=NULL) for (nparameters=0;args[nparameters];++nparameters);

  mtype->name = strdup(name);
  mtype->nparameters = nparameters;
  if (nparameters > 0) {
    mtype->pnames = (const char**)malloc(sizeof(char*) * nparameters);
    mtype->types = (const char**)malloc(sizeof(char*) * nparameters);
  } else {
    mtype->pnames = NULL;
    mtype->types = NULL;
  }
  if (args!=NULL) for (i=0;args[i];++i) {
    const char * x = args[i];
    const char * spos = strchr(x, ':');
    if (spos==NULL) {
      mtype->pnames[i] = strdup(x);
      mtype->types[i] = NULL;
    } else {
      char * cp = strncpy((char*)malloc(spos-x+1), x, spos-x);
      cp[spos-x] = '\0';
      mtype->pnames[i] = cp;
      /* optimierung: Typ-Strings zentral verwalten. */
      mtype->types[i] = strdup(spos+1);
    }
  }
  return mtype;
}

message_type *
mt_new_va(const char * name, ...)
{
	const char * args[16];
	int i = 0;
	va_list marker;

	va_start(marker, name);
	for (;;) {
		const char * c = va_arg(marker, const char*);
		args[i++] = c;
		if (c==NULL) break;
	}
	va_end(marker);
	return mt_new(name, args);
}

typedef struct arg_type {
  struct arg_type * next;
  const char * name;
  void  (*free)(void*);
  void* (*copy)(void*);
} arg_type;

arg_type * argtypes = NULL;

void
register_argtype(const char * name, void(*free_arg)(void*), void*(*copy_arg)(void*))
{
  arg_type * atype = (arg_type *)malloc(sizeof(arg_type));
  atype->name = name;
  atype->next = argtypes;
  atype->free = free_arg;
  atype->copy = copy_arg;
  argtypes = atype;
}

static arg_type *
find_argtype(const char * name)
{
  arg_type * atype = argtypes;
  while (atype!=NULL) {
    if (strcmp(atype->name, name)==0) return atype;
    atype = atype->next;
  }
  return NULL;
}

static void *
copy_arg(const char * type, void * data)
{
  arg_type * atype = find_argtype(type);
  if (atype==NULL) return data;
  if (atype->copy==NULL) return data;
  return atype->copy(data);
}

static void
free_arg(const char * type, void * data)
{
  arg_type * atype = find_argtype(type);
  if (atype && atype->free) atype->free(data);
}

message *
msg_create(const struct message_type * type, void * args[])
{
  int i;
  message * msg = (message *)malloc(sizeof(message));

  assert(type!=NULL);
  if (type==NULL) {
    log_error(("Trying to create message with type=0x0\n"));
    return NULL;
  }
  msg->type = type;
  msg->parameters = (void**)calloc(sizeof(void*), type->nparameters);
  msg->refcount=1;
  for (i=0;i!=type->nparameters;++i) {
    msg->parameters[i] = copy_arg(type->types[i], args[i]);
  }
  return msg;
}

message *
msg_create_va(const struct message_type * type, ...)
/* sets a messages parameters */
{
	void * args[16];
	va_list marker;
	int i;
	va_start(marker, type);
	for (i=0;i!=type->nparameters;++i) {
		args[i] = va_arg(marker, void*);
	}
	va_end(marker);
	return msg_create(type, args);
}

typedef struct messagetype_list {
	struct messagetype_list * next;
	const struct message_type * data;
} messagetype_list;

#define MT_MAXHASH 1021
static messagetype_list * messagetypes[MT_MAXHASH];

const message_type *
mt_find(const char * name)
{
  const struct message_type * found = NULL;
  unsigned int hash = hashstring(name) % MT_MAXHASH;
  messagetype_list * type = messagetypes[hash];
  while (type) {
    if (strcmp(type->data->name, name)==0) {
      if (found==NULL) found = type->data;
      break;
    }
    type = type->next;
  }
  return found;
}


const message_type *
mt_register(const message_type * type)
{
  unsigned int hash = hashstring(type->name) % MT_MAXHASH;
  messagetype_list * mtl = messagetypes[hash];
  while (mtl && mtl->data!=type) mtl=mtl->next;
  if (mtl==NULL) {
    mtl = (messagetype_list*)malloc(sizeof(messagetype_list));
    mtl->data = type;
    mtl->next = messagetypes[hash];
    messagetypes[hash] = mtl;
  }
  return type;
}

void
msg_free(message *msg)
{
  int i;
  assert(msg->refcount==0);
  for (i=0;i!=msg->type->nparameters;++i) {
    free_arg(msg->type->types[i], msg->parameters[i]);
  }
  free((void*)msg->parameters);
  free(msg);
}

void 
msg_release(struct message * msg)
{
  assert(msg->refcount>0);
  if (--msg->refcount>0) return;
  msg_free(msg);
}

struct message * 
msg_addref(struct message * msg)
{
  assert(msg->refcount>0);
  ++msg->refcount;
  return msg;
}

