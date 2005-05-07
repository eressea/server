/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "eressea.h"
#include "message.h"

/* kernel includes */
#include "building.h"
#include "faction.h"
#include "item.h"
#include "order.h"
#include "plane.h"
#include "region.h"
#include "unit.h"

/* util includes */
#include <util/base36.h>
#include <util/goodies.h>
#include <util/message.h>
#include <util/nrmessage.h>
#include <util/crmessage.h>
#include <util/log.h>

/* libc includes */
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

typedef struct msg_setting {
	struct msg_setting *next;
	const struct message_type *type;
	int level;
} msg_setting;

/************ Compatibility function *************/
#define MAXSTRLEN (4*DISPLAYSIZE+3)
#include "region.h"
#include "eressea.h"

messageclass * msgclasses;

const messageclass *
mc_find(const char * name)
{
	messageclass ** mcp = &msgclasses;
	if (name==NULL) return NULL;
	for (;*mcp;mcp=&(*mcp)->next) {
		messageclass * mc = *mcp;
		if (!strcmp(mc->name, name)) break;
	}
	return *mcp;
}

const messageclass *
mc_add(const char * name)
{
	messageclass ** mcp = &msgclasses;
	if (name==NULL) return NULL;
	for (;*mcp;mcp=&(*mcp)->next) {
		messageclass * mc = *mcp;
		if (!strcmp(mc->name, name)) break;
	}
	if (!*mcp) {
		messageclass * mc = calloc(sizeof(messageclass), 1);
		mc->name = strdup(name);
		*mcp = mc;
	}
	return *mcp;
}

static void
arg_set(void * args[], const message_type * mtype, const char * buffer, void * v)
{
	int i;
	for (i=0;i!=mtype->nparameters;++i) {
		if (!strcmp(buffer, mtype->pnames[i])) break;
	}
  if (i!=mtype->nparameters) {
    assert(args[i]==NULL);
    args[i] = v;
  } else {
		fprintf(stderr, "invalid parameter %s for message type %s\n", buffer, mtype->name);
		assert(!"program aborted.");
	}
}

struct message * 
msg_feedback(const struct unit * u, struct order * ord, const char * name, const char* sig, ...)
{
	va_list marker;
	const message_type * mtype = mt_find(name);
	char buffer[64], *oc = buffer;
	const char *ic = sig;
	void * args[16];
	memset(args, 0, sizeof(args));

	if (ord==NULL) ord = u->thisorder;

	if (!mtype) {
		log_error(("trying to create message of unknown type \"%s\"\n", name));
		return NULL;
	}

	arg_set(args, mtype, "unit", (void*)u);
	arg_set(args, mtype, "region", (void*)u->region);
	arg_set(args, mtype, "command", (void*)ord);

	va_start(marker, sig);
	while (*ic && !isalnum(*ic)) ic++;
	while (*ic) {
		void * v = va_arg(marker, void *);
		oc = buffer;
		while (isalnum(*ic)) *oc++ = *ic++;
		*oc = '\0';
		arg_set(args, mtype, buffer, v);
		while (*ic && !isalnum(*ic)) ic++;
	}
	va_end(marker);

	return msg_create(mtype, (void**)args);
}

message * 
msg_message(const char * name, const char* sig, ...)
	/* msg_message("oops_error", "unit region command", u, r, cmd) */
{
	va_list marker;
	const message_type * mtype = mt_find(name);
	char buffer[64], *oc = buffer;
	const char *ic = sig;
	void * args[16];
	memset(args, 0, sizeof(args));

	if (!mtype) {
		log_warning(("trying to create message of unknown type \"%s\"\n", name));
		return NULL;
	}

	va_start(marker, sig);
	while (*ic && !isalnum(*ic)) ic++;
	while (*ic) {
		void * v = va_arg(marker, void *);
		oc = buffer;
		while (isalnum(*ic)) *oc++ = *ic++;
		*oc = '\0';
		arg_set(args, mtype, buffer, v);
		while (*ic && !isalnum(*ic)) ic++;
	}
	va_end(marker);

	return msg_create(mtype, (void**)args);
}

message *
new_message(struct faction * receiver, const char* sig, ...)
	/* compatibility function, to keep the old function calls valid *
	 * all old messagetypes are converted into a message with ONLY string parameters,
	 * this function will convert given parameters to a string representation
	 * based on the signature - like render() once did */
{
	const message_type * mtype;
	va_list marker;
	const char * signature = strchr(sig, '%');
	char buffer[128];
	int i=0;
	const char * c = sig;
	void * args[16];

	memset(args, 0, sizeof(args));
	assert(signature-sig < sizeof(buffer));
	strnzcpy(buffer, sig, signature-sig);
	mtype = mt_find(buffer);

	if (!mtype) {
		log_error(("trying to create message of unknown type \"%s\"\n", buffer));
		return NULL;
	}

	while(*c!='%') buffer[i++] = *(c++);
	buffer[i] = 0;

	va_start(marker, sig);
	while (*c) {
		char type;
		char *p = buffer;
		assert(*c=='%');
		type = *(++c);
	/*
			case 'f': (*ep)->type = IT_FACTION; break;
			case 'u': (*ep)->type = IT_UNIT; break;
			case 'r': (*ep)->type = IT_REGION; break;
			case 'h': (*ep)->type = IT_SHIP; break;
			case 'b': (*ep)->type = IT_BUILDING; break;
			case 'X': (*ep)->type = IT_RESOURCETYPE; break;
			case 'x': (*ep)->type = IT_RESOURCE; break;
			case 't': (*ep)->type = IT_SKILL; break;
			case 's': (*ep)->type = IT_STRING; break;
			case 'i': (*ep)->type = IT_INT; break;
			case 'd': (*ep)->type = IT_DIRECTION; break;
			case 'S': (*ep)->type = IT_FSPECIAL; break;
*/
		c+=2;
		while (*c && isalnum(*(unsigned char*)c)) *(p++) = *(c++);
		*p = '\0';
		for (i=0;i!=mtype->nparameters;++i) {
			if (!strcmp(buffer, mtype->pnames[i])) break;
		}
		if (i==mtype->nparameters) {
			log_error(("unknown message-parameter for %s: %s (%p)\n", mtype->name, buffer, va_arg(marker, void*)));
			continue;
		}
		switch(type) {
			case 's':
				args[i] = (void*)va_arg(marker, const char *);
				break;
			case 'i':
				args[i] = (void*)va_arg(marker, int);
				break;
			case 'f':
				args[i] = (void*)va_arg(marker, const struct faction*);
				break;
			case 'u':
				args[i] = (void*)va_arg(marker, const struct unit*);
				break;
			case 'r':
				args[i] = (void*)va_arg(marker, const struct region*);
				break;
			case 'h':
				args[i] = (void*)va_arg(marker, const struct ship*);
				break;
			case 'b':
				args[i] = (void*)va_arg(marker, const struct building*);
				break;
			case 'X':
				args[i] = (void*)va_arg(marker, const resource_type *);
				break;
			case 'x':
				args[i] = (void*)oldresourcetype[(resource_t)va_arg(marker, int)];
				break;
			case 't':
				args[i] = (void*)va_arg(marker, int);
				break;
			case 'd':
				args[i] = (void*)i;
				break;
			case 'S':
			default:
				args[i] = NULL;
		}
	}
	va_end(marker);
	return msg_create(mtype, (void**)args);
}

static void
caddmessage(region * r, faction * f, const char *s, msg_t mtype, int level)
{
	message * m = NULL;

	unused(level);
	switch (mtype) {
	case MSG_INCOME:
		assert(f);
		m = add_message(&f->msgs, msg_message("msg_economy", "string", s));
		break;
	case MSG_BATTLE:
		assert(0 || !"battle-meldungen nicht über addmessage machen");
		break;
	case MSG_MOVE:
		assert(f);
		m = add_message(&f->msgs, msg_message("msg_movement", "string", s));
		break;
	case MSG_COMMERCE:
		assert(f);
		m = add_message(&f->msgs, msg_message("msg_economy", "string", s));
		break;
	case MSG_PRODUCE:
		assert(f);
		m = add_message(&f->msgs, msg_message("msg_production", "string", s));
		break;
	case MSG_MAGIC:
	case MSG_COMMENT:
	case MSG_MESSAGE:
	case MSG_ORCVERMEHRUNG:
	case MSG_EVENT:
		/* Botschaften an REGION oder einzelne PARTEI */
		m = msg_message("msg_event", "string", s);
		if (!r) {
			assert(f);
			m = add_message(&f->msgs, m);
		} else {
			if (f==NULL) add_message(&r->msgs, m);
			else r_addmessage(r, f, m);
		}
		break;
	default:
		assert(!"Ungültige Msg-Klasse!");
	}
	if (m) msg_release(m);
}

void
addmessage(region * r, faction * f, const char *s, msg_t mtype, int level)
{
  caddmessage(r, f, gc_add(strdup(s)), mtype, level);
}

void
mistake(const unit * u, struct order * ord, const char *comment, int mtype)
{
  if (u->faction->no != MONSTER_FACTION) {
    char * cmt = strdup(comment);
    ADDMSG(&u->faction->msgs, msg_message("mistake",
      "command error unit region", duplicate_order(ord), cmt, u, u->region));
  }
}

void
cmistake(const unit * u, struct order *ord, int mno, int mtype)
{
	static char ebuf[20];
  unused(mtype);

	if (u->faction->no == MONSTER_FACTION) return;
	sprintf(ebuf, "error%d", mno);
	ADDMSG(&u->faction->msgs, msg_message(ebuf, 
		"command unit region", duplicate_order(ord), u, u->region));
}

extern unsigned int new_hashstring(const char* s);

void
free_messagelist(message_list * msgs)
{
  struct mlist ** mlistptr = &msgs->begin;
  while (*mlistptr) {
    struct mlist * ml = *mlistptr;
    *mlistptr = ml->next;
    msg_release(ml->msg);
    free(ml);
  }
  free(msgs);
}

message * 
add_message(message_list** pm, message * m)
{
	if (!lomem && m!=NULL) {
		struct mlist * mnew = malloc(sizeof(struct mlist));
		if (*pm==NULL) {
			*pm = malloc(sizeof(message_list));
			(*pm)->end=&(*pm)->begin;
		}
		mnew->msg = msg_addref(m);
		mnew->next = NULL;
		*((*pm)->end) = mnew;
		(*pm)->end=&mnew->next;
	}
	return m;
}


