/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 $Id: crmessage.c,v 1.2 2001/04/12 17:21:45 enno Exp $
*/

#include <config.h>
#include "crmessage.h"

#include "message.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/** type to string conversion **/
typedef struct tsf_list {
	struct tsf_list * next;
	const char * name;
	tostring_f fun;
} tsf_list;

static tsf_list * tostringfs;

static tostring_f
tsf_find(const char * name)
{
	if (name!=NULL) {
		tsf_list * tsf;
		for (tsf=tostringfs;tsf;tsf=tsf->next) {
			if (!strcmp(tsf->name, name)) return tsf->fun;
		}
	}
	return NULL;
}

void
tsf_register(const char * name, tostring_f fun)
{
	tsf_list * tsf;
	for (tsf=tostringfs;tsf;tsf=tsf->next) {
		if (!strcmp(tsf->name, name)) break;
	}
	if (tsf==NULL) {
		tsf = malloc(sizeof(tsf_list));
		tsf->fun = fun;
		tsf->name = name;
		tsf->next = tostringfs;
		tostringfs = tsf;
	}
}

/** crmesssage **/
typedef struct crmessage_type {
	const struct message_type * mtype;
	tostring_f * renderers;
	struct crmessage_type * next;
} crmessage_type;

static crmessage_type * messagetypes;

static crmessage_type * 
crt_find(const struct message_type * mtype)
{
	crmessage_type * found = NULL;
	crmessage_type * type = messagetypes;
	while (type) {
		if (type->mtype==mtype) found = type;
		type = type->next;
	}
	return found;
}

void
crt_register(const struct message_type * mtype)
{
	crmessage_type * crt = messagetypes;
	while (crt && crt->mtype!=mtype) {
		crt = crt->next;
	}
	if (!crt) {
		int i;
		crt = malloc(sizeof(crmessage_type));
		crt->mtype = mtype;
		crt->next = messagetypes;
		messagetypes = crt;
		crt->renderers = malloc(sizeof(tostring_f)*mtype->nparameters);

		/* TODO: can be scrapped for memory vs. speed */
		for (i=0;i!=mtype->nparameters;++i) {
			crt->renderers[i] = tsf_find(mtype->types[i]);
		}
	}
}

int
cr_render(const message * msg, char * buffer, const void * userdata)
{
	int i;
	char * c = buffer;
	struct crmessage_type * crt = crt_find(msg->type);

	if (crt==NULL) return -1;
	for (i=0;i!=msg->type->nparameters;++i) {
		if (crt->renderers[i]==NULL) {
			log_error(("No renderer for argument %s:%s of \"%s\"\n", 
				msg->type->pnames[i], msg->type->types[i], msg->type->name));
			continue; /* strcpy(c, (const char*)msg->parameters[i]); */
		} else {
			if (crt->renderers[i](msg->parameters[i], c, userdata)!=0) continue;
		}
		c += strlen(c);
		sprintf(c, ";%s\n", msg->type->pnames[i]);
		c += strlen(c);
	}
	return 0;
}

int
cr_string(const void * v, char * buffer, const void * userdata)
{
	sprintf(buffer, "\"%s\"", (const char *)v);
	unused(userdata);
	return 0;
}

int
cr_int(const void * v, char * buffer, const void * userdata)
{
	sprintf(buffer, "%d", (int)v);
	unused(userdata);
	return 0;
}

int
cr_ignore(const void * v, char * buffer, const void * userdata)
{
	unused(v);
	unused(buffer);
	unused(userdata);
	return -1;
}
