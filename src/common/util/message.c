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
	int i, nparameters;
	message_type * mtype = (message_type *)malloc(sizeof(message_type));

	for (nparameters=0;args[nparameters];++nparameters);

	mtype->name = strdup(name);
	mtype->nparameters = nparameters;
	if(nparameters > 0) {
		mtype->pnames = (const char**)malloc(sizeof(char*) * nparameters);
		mtype->types = (const char**)malloc(sizeof(char*) * nparameters);
	} else {
		mtype->pnames = NULL;
		mtype->types = NULL;
	}
	for (i=0;args[i];++i) {
		const char * x = args[i];
		const char * spos = strchr(x, ':');
		if (spos==NULL) {
			mtype->pnames[i] = strdup(x);
			mtype->types[i] = NULL;
		} else {
			char * cp = strncpy(malloc(spos-x+1), x, spos-x);
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

message *
msg_create(const struct message_type * type, void * args[])
{
	int i;
	message * msg = (message *)malloc(sizeof(message));
	msg->type = type;
	msg->parameters = calloc(sizeof(void*), type->nparameters);
	msg->refcount=1;
	for (i=0;i!=type->nparameters;++i) {
		msg->parameters[i] = args[i];
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

#define MT_MAXHASH 1023
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
		mtl = malloc(sizeof(messagetype_list));
		mtl->data = type;
		mtl->next = messagetypes[hash];
		messagetypes[hash] = mtl;
	}
	return type;
}

void
msg_free(message *msg)
{
	assert(msg->refcount==0);
	free((void*)msg->parameters);
	free(msg);
}

void 
msg_release(struct message * msg)
{
	if (--msg->refcount>0) return;
	msg_free(msg);
}

struct message * 
msg_addref(struct message * msg)
{
	++msg->refcount;
	return msg;
}

