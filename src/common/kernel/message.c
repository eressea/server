/* vi: set ts=2:
 *
 *	$Id: message.c,v 1.11 2001/03/01 01:38:12 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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

/** defines:
 *
 * NEW_MESSAGES - new message-classes
 *
 */
#include <config.h>
#include "eressea.h"
#include "message.h"

/* kernel includes */
#include "plane.h"
#include "faction.h"
#include "unit.h"
#include "item.h"
#include "building.h"

/* util includes */
#include <goodies.h>
#include <base36.h>
#ifdef NEW_MESSAGES
#include <message.h>
#include <nrmessage.h>
#include <crmessage.h>
#endif

/* libc includes */
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

typedef struct warning {
	struct warning * next;
	const struct messageclass * section;
	int level;
} warning;

typedef struct msg_setting {
	struct msg_setting *next;
	const struct message_type *type;
	int level;
} msg_setting;

/************ Compatibility function *************/
#define MAXSTRLEN (4*DISPLAYSIZE+3)
#include "region.h"
#include "eressea.h"
const char *
translate_regions(const char *st, faction * f)
{
	static char temp[MAXSTRLEN - 1];
	char *s, *t = temp;
	const char *p = st;
	char *c = strstr(p, "\\r(");

	if (!c) return strcpy(temp, st);

	temp[0] = 0;
	do {
		static region *r;
		static int cache_x = -999999, cache_y = -999999;
		int koor_x, koor_y;
		int x = MAXSTRLEN - (t - temp);
		plane *cache_pl = NULL;

		if (c - p < x)
			x = c - p;

		s = temp;
		strncpy(t, p, x);
		t += (c - p);
		p = c + 3;
		koor_x = atoi(p);
		p  = strchr(p, ',') + 1;
		koor_y = atoi(p);

		if (koor_x != cache_x || koor_y != cache_y) {
			r = findregion(koor_x, koor_y);
			cache_x = koor_x;
			cache_y = koor_y;
			cache_pl = getplane(r);
		}

		if (f == NULL)
			f = findfaction(MONSTER_FACTION);

		if(r) {
			if (cache_pl && fval(cache_pl,PFL_NOCOORDS)) {
				sprintf(t, "%s", rname(r, f->locale));
			} else {
				const char *rn = rname(r, f->locale);
				if(rn && *rn) {
					sprintf(t, "%s (%d,%d)", rn, region_x(r, f), region_y(r, f));
				} else {
					sprintf(t, "(%d,%d)", region_x(r, f), region_y(r, f));
				}
			}
		} else strcpy(t, "(Chaos)");

		t += strlen(t);
		p = strchr(p, ')') + 1;
		c = strstr(p, "\\r(");
	} while (c!= NULL);
	if (s == temp)
		strcat(t, p);
	return s;
}

#ifdef NEW_MESSAGES
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
	const char * args[16];

	memset(args, 0, sizeof(args));
	strncpy(buffer, sig, signature-sig);
	buffer[signature-sig] = '\0';
	mtype = mt_find(buffer);

	if (!mtype) {
		fprintf(stderr, "trying to create message of unknown type \"%s\"\n", buffer);
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
				args[i] = va_arg(marker, const char *);
				break;
			case 'i':
				args[i] = strdup(itoa10(va_arg(marker, int)));
				break;
			case 'f':
				args[i] = factionname(va_arg(marker, const struct faction*));
				break;
			case 'u': {
				const struct unit * u = va_arg(marker, const struct unit*);
				if (u) args[i] = unitname(u);
			}
			break;
			case 'r': {
				const struct region * r = va_arg(marker, const struct region*);
				if (r) args[i] = rname(r, receiver->locale);
			}
			break;
			case 'h': {
				const struct ship * sh = va_arg(marker, const struct ship*);
				if (sh) args[i] = shipname(sh);
			}
			break;
			case 'b': {
				const struct building * b = va_arg(marker, const struct building*);
				if (b) args[i] = buildingname(b);
			}
			break;
			case 'X':
				args[i] = resourcename(va_arg(marker, const resource_type *), 0);
				break;
			case 'x':
				args[i] = resourcename(oldresourcetype[(resource_t)va_arg(marker, resource_t)], 0);
				break;
			case 't':
				args[i] = skillnames[va_arg(marker, skill_t)];
				break;
			case 'd':
				args[i] = directions[i];
				break;
			case 'S':
			default:
				args[i] = NULL;
		}
	}
	return msg_create(mtype, (void**)args);
}

static void
parse_message(char * b, const struct locale * deflocale)
{
	char *m, *a = NULL, message[8192];
	char * name;
	char * language;
	const struct locale * lang;
	char * section = NULL;
	int i, level = 0;
	char * args[16];
	boolean f_symbol = false;
	const struct message_type * mtype;

	/* skip comments */
	if (b[0]=='#' || b[0]==0) return;

	/* the name of this type */
	name = b;
	while (*b && *b!=';') ++b;
	if (!*b) return;
	*b++ = 0;

	/* the section for this type */
	section = b;
	while (*b && *b!=';' && *b!=':') ++b;
	if (!strcmp(section, "none")) section=NULL;

	/* if available, the level for this type */
	if (*b==':') {
		char * x;
		*b++ = 0;
		x = b;
		while (*b && *b!=';') ++b;
		level=atoi(x);
	}
	*b++ = 0;

	/* the locale */
	language = b;
	while (*b && *b!=';') ++b;
	*b++ = 0;
	if (strlen(language)==0) lang = deflocale;
	else {
		lang = find_locale(language);
		if (!lang) lang = make_locale(language);
	}
	/* parse the message */
	i = 0;
	m = message;
	*m++='\"';
	while (*b) {
		switch (*b) {
		case '{':
			f_symbol = true;
			a = ++b;
			break;
		case '}':
			*b++ = '\0';
			args[i] = strdup(a);
			sprintf(m, "$%s", args[i]);
			m+=strlen(m);
			i++;
			f_symbol = false;
			break;
		case ' ':
			if (f_symbol) {
				a = ++b;
				break;
			}
			/* fall-through intended */
		default:
			if (!f_symbol) {
				*m++ = *b++;
			} else b++;
		}
	}
	strcpy(m, "\"");
	args[i] = NULL;

	/* add the messagetype */
	mtype = mt_register(mt_new(name, (const char**)args));
	nrt_register(mtype, lang, message, level, section);
	crt_register(mtype, lang);
}

void
read_messages(FILE * F, const struct locale * lang)
{
	char buf[8192];
	while (fgets(buf, sizeof(buf), F)) {
		buf[strlen(buf)-1] = 0; /* \n weg */
		parse_message(buf, lang);
	}
}

#endif

void
addmessage(region * r, faction * f, const char *s, msg_t mtype, int level)
{
	caddmessage(r, f, gc_add(strdup(translate_regions(s, f))), mtype, level);
}

void
caddmessage(region * r, faction * f, char *s, msg_t mtype, int level)
{
	message * m = NULL;
	switch (mtype) {
	case MSG_INCOME:
		assert(f);
		m = add_message(&f->msgs, new_message(f, "msg_economy%s:string", s));
		break;
	case MSG_BATTLE:
		assert(0 || !"battle-meldungen nicht über addmessage machen");
		break;
	case MSG_MOVE:
		assert(f);
		m = add_message(&f->msgs, new_message(f, "msg_movement%s:string", s));
		break;
	case MSG_COMMERCE:
		assert(f);
		m = add_message(&f->msgs, new_message(f, "msg_economy%s:string", s));
		break;
	case MSG_PRODUCE:
		assert(f);
		m = add_message(&f->msgs, new_message(f, "msg_production%s:string", s));
		break;
	case MSG_MAGIC:
	case MSG_COMMENT:
	case MSG_MESSAGE:
		/* Botschaften an REGION oder einzelne PARTEI */
		if (!r)
			m = add_message(&f->msgs, new_message(f, "msg_event%s:string", s));
		else
			m = add_message(&r->msgs, new_message(f, "msg_event%s:string", s));
		break;
	case MSG_ORCVERMEHRUNG:
	case MSG_EVENT:
		/* Botschaften an REGION oder einzelne PARTEI */
		if (!r)
			m = add_message(&f->msgs, new_message(f, "msg_event%s:string", s));
		else
			m = add_message(&r->msgs, new_message(f, "msg_event%s:string", s));
		break;
	default:
		fprintf(stderr, "Warnung: Ungültige Msg-Klasse!");
	}
#ifdef OLD_MESSAGES
	if (m) m->level = level;
#endif
}

void
xmistake(const unit * u, const char *s, const char *comment, int mtype)
{
	if (u->faction->no == MONSTER_FACTION) return;
	add_message(&u->faction->msgs, new_message(u->faction, "mistake%s:command%s:error%u:unit%r:region", s, comment, u, u->region));
}

void
cmistake(const unit * u, const char *cmd, int mno, int mtype)
{
	static char lbuf[64];
	if (u->faction->no == MONSTER_FACTION) return;
	sprintf(lbuf, "error%d", mno);
	strcat(lbuf, "%s:command%i:errno%u:unit%r:region");
	add_message(&u->faction->msgs, new_message(u->faction, lbuf, cmd, mno, u, u->region));
}

void
mistake(const unit * u, const char *command, const char *comment, int mtype)
{
	if (u->faction->no == MONSTER_FACTION) return;
	xmistake(u, command, gc_add(strdup(translate_regions(comment, u->faction))), mtype);
}

extern unsigned int new_hashstring(const char* s);

int
old_hashstring(const char* s)
{
	int key = 0;
	int i = strlen(s);
	while (i) {
		--i;
		key = ((key >> 31) & 1) ^ (key << 1) ^ s[i];
	}
	return key & 0x7fff;
}


#ifdef OLD_MESSAGES
static messagetype * messagetypes;

void
debug_messagetypes(FILE * out)
{
	messagetype * mt;
	for (mt=messagetypes;mt;mt=mt->next) {
		fprintf(out, "%ut%u\n", old_hashstring(mt->name), mt->hashkey);
	}
}

messagetype *
new_messagetype(const char * name, int level, const char * section)
{
	messagetype * mt = calloc(1, sizeof(messagetype));
	mt->section = mc_add(section);
	mt->name = strdup(name);
	mt->level = level;
	mt->hashkey = hashstring(mt->name);
#ifndef NDEBUG
	{
		messagetype * mt2 = messagetypes;
		while(mt2 && mt2->hashkey != mt->hashkey) mt2 = mt2->next;
		if (mt2) {
			fprintf(stderr, "duplicate hashkey %u for messagetype %s and %s\n",
				mt->hashkey, name, mt2->name);
		}
	}
#endif
	mt->next = messagetypes;
	messagetypes = mt;
	return mt;
}

void
add_signature(messagetype * mt, const char * sig)
{
	static char buffer[128];
	int i=0;
	struct entry ** ep = &mt->entries;
	const char * c = sig;

	if (mt->entries) return;
	while(*c!='%') buffer[i++] = *(c++);
	buffer[i] = 0;

	while (*c) {
		char *p = buffer;
		assert(*c=='%');
		*ep = calloc(1, sizeof(struct entry));
		switch (*(++c)) {
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
		}
		++c;
		assert(*c==':');
		++c;
		while (*c && isalnum((int)*c))
			*(p++) = *(c++);
		*p = 0;
		(*ep)->name = strdup(buffer);
		ep = &(*ep)->next;
		++mt->argc;
	}
}

messagetype *
find_messagetype(const char * name)
{
	messagetype * mt = messagetypes;
	while(mt && strcmp(mt->name, name)!=0) mt = mt->next;
	return mt;
}

message *
new_message(struct faction * receiver, const char* sig, ...)
{
	message * m = (message*)calloc(1, sizeof(message));
	messagetype * mt;
	struct entry * e;
	va_list marker;
	int i;
	unit * u;
	const char * signature = strchr(sig, '%');
	char name[64];

	assert(signature-sig<64);
	memcpy(name, sig, signature-sig);
	name[signature-sig] = 0;

	mt = find_messagetype(name);
	if (!mt) {
		fprintf(stderr, "unknown message %s\n", name);
		return NULL;
	}
	if (mt->entries==NULL) add_signature(mt, signature);
	m->receiver = receiver;
	m->type = mt;
	m->data = calloc(mt->argc, sizeof(void*));
	m->level = mt->level;
	e = mt->entries;

	va_start(marker, sig);
	for (i=0;i!=mt->argc;++i) {
		switch (e->type) {
			case IT_INT:
				m->data[i] = (void*)va_arg(marker, int);
				break;
			case IT_RESOURCETYPE:
				m->data[i] = (void*)va_arg(marker, const resource_type *);
				break;
			case IT_RESOURCE:
				m->data[i] = (void*)oldresourcetype[(resource_t)va_arg(marker, resource_t)];
				break;
			case IT_UNIT:
				u = (unit*)va_arg(marker, void*);
				/* TODO: Ergibt keine richtige Grammatik */
				if (u == NULL)
					m->data[i] = NULL;
				else {
					m->data[i] = u;
				}
#if 0
				if (u && (!receiver || cansee(receiver, u->region, u, 0)))
					m->data[i] = u;
				else
					m->data[i] = NULL;
#endif
				break;
			default:
				m->data[i] = va_arg(marker, void*);
				break;
		}
		e = e->next;
	}
	assert(!e);
	va_end(marker);
	return m;
}

int
get_msglevel(const struct warning * warnings, const msglevel * levels, const messagetype * mtype)
{
#ifdef MSG_LEVELS
	/* hier ist ein bug drin, irgendwo */
	const struct warning * w = warnings;
	while (levels && levels->type!=mtype) levels=levels->next;
	if (levels) return levels->level;
	while (w) {
		if (w->section == mtype->section) break;
		w = w->next;
	}
	if (w) return w->level;
#endif
	return 0x7F;
}

int
msg_level(const message * m)
{
	return m->level;
}
#endif

void
set_msglevel(struct warning ** warnings, const char * type, int level)
{
	struct warning ** w = warnings;
	while (*w) {
		if (!strcasecmp((*w)->section->name, type)) break;
		w = &(*w)->next;
	}
	if (*w && level==-1){
		struct warning * old = *w;
		*w = old->next;
		free(old);
	} else if (level>=0) {
		struct warning * x = calloc(sizeof(struct warning), 1);
		x->next = *w;
		x->level = level;
		x->section = mc_find(type);
		*w = x;
	}
}

message * 
add_message(message_list** pm, message * m)
{
	if (m==NULL) return NULL;
#ifdef NEW_MESSAGES
	else {
		struct mlist * mnew = malloc(sizeof(struct mlist));
		if (*pm==NULL) {
			*pm = malloc(sizeof(message_list));
			(*pm)->end=&(*pm)->begin;
		}
		mnew->msg = m;
		mnew->next = NULL;
		*((*pm)->end) = mnew;
		(*pm)->end=&mnew->next;
	}
#else
	m->next = *pm;
	*pm = m;
#endif
	return m;
}

void
free_messages(message_list * m)
{
#ifdef NEW_MESSAGES
	struct mlist * x = m->begin;
	while (x) {
		m->begin = x->next;
		msg_free(x->msg);
		free(x);
	}
#else
	while (m) {
		message_list * x = m;
		m = x->next;
		free(x->data);
		free(x);
	}
#endif
}

messageclass * msgclasses;

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

void
write_msglevels(struct warning * warnings, FILE * F)
{
#ifdef MSG_LEVELS
	/* hier ist ein bug? */
	struct warning * w = warnings;
	while (w) {
		fprintf(F, "%s %d ", w->section->name, w->level);
		w = w->next;
	}
#endif
	fputs("end ", F);
}

void
read_msglevels(struct warning ** w, FILE * F)
{
	fscanf(F, "%s", buf);
	while (strcmp("end", buf)) {
		int level;
		const messageclass * mc = mc_find(buf);
		fscanf(F, "%d ", &level);
		if (mc && level>=0) {
			*w = calloc(sizeof(struct warning), 1);
			(*w)->level = level;
			(*w)->section = mc;
			w = &(*w)->next;
		}
		fscanf(F, "%s", buf);
	}
}
