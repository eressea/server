/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 $Id: message.c,v 1.1 2001/02/24 12:50:50 enno Exp $
*/

#include <config.h>
#include "message.h"

#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

message_type * 
mt_new(const char * name, const char * args[])
{
	int i, nparameters;
	message_type * mtype = (message_type *)malloc(sizeof(message_type));

	for (nparameters=0;args[nparameters];++nparameters);

	mtype->name = strdup(name);
	mtype->nparameters = nparameters;
	mtype->pnames = (char**)malloc(sizeof(char*) * nparameters);
	mtype->types = (char**)malloc(sizeof(char*) * nparameters);
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
	const message_type * type;
} messagetype_list;

static messagetype_list * messagetypes;

const message_type * 
mt_register(const message_type * type)
{
	messagetype_list * mtl = messagetypes;
	while (mtl && mtl->type!=type) mtl=mtl->next;
	if (mtl==NULL) {
		mtl = malloc(sizeof(messagetype_list));
		mtl->type = type;
		mtl->next = messagetypes;
		messagetypes = mtl;
	}
	return type;
}

const message_type * 
mt_find(const char * name)
{
	messagetype_list * mtl = messagetypes;
	while (mtl && strcmp(mtl->type->name, name)!=0) mtl=mtl->next;
	return mtl?mtl->type:NULL;
}

void
msg_free(message *m)
{
	free((void*)m->parameters);
	free(m);
}