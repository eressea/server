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
#include "nrmessage.h"
#include "nrmessage_struct.h"

/* util includes */
#include "log.h"
#include "message.h"
#include "language.h"
#include "translation.h"

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static nrmessage_type * messagetypes;

nrmessage_type * get_nrmessagetypes(void) {
	return messagetypes;
}

const char * 
nrt_string(const struct nrmessage_type *type)
{
	return type->string;
}

nrmessage_type * 
nrt_find(const struct locale * lang, const struct message_type * mtype)
{
	nrmessage_type * found = NULL;
	nrmessage_type * type = messagetypes;
	while (type) {
		if (type->mtype==mtype) {
			if (found==NULL) found = type;
			else if (type->lang==NULL) found = type;
			if (lang==type->lang) {
				found = type;
				break;
			}
		}
		type = type->next;
	}
	return found;
}

void 
nrt_register(const struct message_type * mtype, const struct locale * lang, const char * string, int level, const char * section)
{
	nrmessage_type * nrt = messagetypes;
	while (nrt && (nrt->lang!=lang || nrt->mtype!=mtype)) {
		nrt = nrt->next;
	}
	if (!nrt) {
		int i;
		char zNames[256];
		char * c = zNames;
		nrt = malloc(sizeof(nrmessage_type));
		nrt->lang = lang;
		nrt->mtype = mtype;
		nrt->next = messagetypes;
		nrt->level=level;
		if (section) nrt->section = strdup(section);
		else nrt->section = NULL;
		messagetypes = nrt;
		assert(string && *string);
		nrt->string = strdup(string);
                *c = '\0';
		for (i=0;i!=mtype->nparameters;++i) {
			if (i!=0) *c++ = ' ';
			c+= strlen(strcpy(c, mtype->pnames[i]));
		}
		nrt->vars = strdup(zNames);
		/* TODO: really necessary to strdup them all? here? better to extend the caller? hash? */
	}
}

int
nr_render(const struct message * msg, const struct locale * lang, char * buffer, size_t bufsize, const void * userdata)
{
	struct nrmessage_type * nrt = nrt_find(lang, msg->type);

	if (nrt) {
		const char * m = translate(nrt->string, userdata, nrt->vars, msg->parameters);
		if (m) {
			strcpy(buffer, m);
			return 0;
		} else {
			log_error(("Couldn't render message %s\n", nrt->mtype->name));
		}
	}
	return -1;
}

int 
nr_level(const struct message *msg)
{
	nrmessage_type * nrt = nrt_find(NULL, msg->type);
	return nrt->level;
}

const char * 
nr_section(const struct message *msg)
{
	nrmessage_type * nrt = nrt_find(default_locale, msg->type);
	return nrt->section;
}

const char * 
nrt_section(const nrmessage_type * nrt)
{
	return nrt->section;
}
