/* vi: set ts=2:
 *
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

#include <config.h>
#include "eressea.h"
#include "message.h"

/* kernel includes */
#include "plane.h"
#include "faction.h"
#include "unit.h"
#include "region.h"
#include "item.h"
#include "building.h"

/* util includes */
#include <base36.h>
#include <goodies.h>
#include <message.h>
#include <nrmessage.h>
#include <crmessage.h>
#include <log.h>

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

#include <xml.h>

typedef struct xml_state {
	const char * mtname;
	const message_type * mtype;
	int argc;
	char * argv[32];
	struct locale * lang;
	const char * nrsection;
	int nrlevel;
	char * nrtext;
} xml_state;

static int 
parse_plaintext(const struct xml_stack *stack, const char *str, void *data)
{
	xml_state * state = (xml_state*)data;
	if (stack) {
		const xml_tag * tag = stack->tag;
		if (strcmp(tag->name, "text")==0) {
			state->nrtext = strdup(str);
		}
	}
	return XML_OK;
}

static int 
parse_tagbegin(const struct xml_stack *stack, void *data)
{
	xml_state * state = (xml_state*)data;
	const xml_tag * tag = stack->tag;
	if (strcmp(tag->name, "messages")==0) {
		memset(state, 0, sizeof(xml_state));
		return XML_OK;
	} else if (strcmp(tag->name, "message")==0) {
		const char * tname = xml_value(tag, "name");
		state->argc = 0;
		if (tname) {
			state->mtname = tname;
			return XML_OK;
		}
		else state->mtname = NULL;
	} else if (strcasecmp(tag->name, "arg")==0) {
		if (state->mtname!=NULL) {
			const char * zName = xml_value(tag, "name");
			const char * zType = xml_value(tag, "type");
			if (zName && zType) {
				char zBuffer[128];
				sprintf(zBuffer, "%s:%s", zName, zType);
				state->argv[state->argc++] = strdup(zBuffer);
				return XML_OK;
			}
		}
	} else if (strcasecmp(tag->name, "locale")==0) {
		if (state->mtname!=NULL) {
			const char * zName = xml_value(tag, "name");
			if (zName) {
				state->lang = find_locale(zName);
			}
		}
	} else if (strcasecmp(tag->name, "nr")==0) {
		if (state->mtname!=NULL) {
			const char * zSection = xml_value(tag, "section");
			const char * zLevel = xml_value(tag, "level");
			if (zSection) {
				state->nrsection = zSection;
				mc_add(zSection);
			}
			if (zLevel) state->nrlevel = atoi(zLevel);
		}
	}
	return XML_USERERROR;
}

static int 
parse_tagend(const struct xml_stack *stack, void *data)
{
	xml_state * state = (xml_state*)data;
	const xml_tag * tag = stack->tag;

	if (strcasecmp(tag->name, "type")==0) {
		const struct message_type * mtype;

		state->argv[state->argc]=0;

		/* add the messagetype */
		mtype = mt_find(state->mtname);
		if (!mtype) mtype = mt_register(mt_new(state->mtname, (const char**)state->argv));
		
		while (state->argc--) {
			free(state->argv[state->argc]); 
		}
		if (state->nrtext) {
			free(state->nrtext);
			state->nrtext = 0;
		}
		state->mtype = mtype;
	} else if (strcasecmp(tag->name, "locale")==0) {
		crt_register(state->mtype);
		state->lang = NULL;
	} else if (strcasecmp(tag->name, "nr")==0) {
		nrt_register(state->mtype, state->lang, state->nrtext, state->nrlevel, state->nrsection);
		state->nrsection = NULL;
	}
	return XML_OK;
}

static xml_callbacks msgcallback = {
	parse_plaintext,
	parse_tagbegin,
	parse_tagend,
	NULL
};

void
read_messages(FILE * F, const locale * lang)
{
	xml_state state;
	xml_parse(F, &msgcallback, &state);

	unused(lang);
}

static void
arg_set(void * args[], const message_type * mtype, const char * buffer, void * v)
{
	int i;
	for (i=0;i!=mtype->nparameters;++i) {
		if (!strcmp(buffer, mtype->pnames[i])) break;
	}
	if (i!=mtype->nparameters) args[i] = v;
}

struct message * 
msg_error(const struct unit * u, const char * cmd, const char * name, const char* sig, ...)
{
	va_list marker;
	const message_type * mtype = mt_find(name);
	char buffer[64], *oc = buffer;
	const char *ic = sig;
	void * args[16];
	memset(args, 0, sizeof(args));

	assert(cmd!=u->thisorder || !"only use entries from u->orders - memory corruption imminent.");
	if (cmd==NULL) cmd = findorder(u, u->thisorder);


	if (!mtype) {
		fprintf(stderr, "trying to create message of unknown type \"%s\"\n", name);
		return NULL;
	}

	arg_set(args, mtype, "unit", (void*)u);
	arg_set(args, mtype, "region", (void*)u->region);
	arg_set(args, mtype, "command", (void*)cmd);

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

	return msg_create(mtype, u->faction, (void**)args);
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
		fprintf(stderr, "trying to create message of unknown type \"%s\"\n", name);
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

	return msg_create(mtype, NULL, (void**)args);
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
				args[i] = (void*)oldresourcetype[(resource_t)va_arg(marker, resource_t)];
				break;
			case 't':
				args[i] = (void*)va_arg(marker, skill_t);
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
	return msg_create(mtype, receiver, (void**)args);
}

static void
caddmessage(region * r, faction * f, const char *s, msg_t mtype, int level)
{
	message * m = NULL;

	unused(level);
	switch (mtype) {
	case MSG_INCOME:
		assert(f);
		add_message(&f->msgs, msg_message("msg_economy", "string", s));
		break;
	case MSG_BATTLE:
		assert(0 || !"battle-meldungen nicht über addmessage machen");
		break;
	case MSG_MOVE:
		assert(f);
		add_message(&f->msgs, msg_message("msg_movement", "string", s));
		break;
	case MSG_COMMERCE:
		assert(f);
		add_message(&f->msgs, msg_message("msg_economy", "string", s));
		break;
	case MSG_PRODUCE:
		assert(f);
		add_message(&f->msgs, msg_message("msg_production", "string", s));
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
			if (f) add_message(&r->msgs, m);
			else r_addmessage(r, f, m);
		}
		break;
	default:
		assert(!"Ungültige Msg-Klasse!");
	}
}

void
addmessage(region * r, faction * f, const char *s, msg_t mtype, int level)
{
	caddmessage(r, f, gc_add(strdup(translate_regions(s, f))), mtype, level);
}

void
xmistake(const unit * u, const char *s, const char *comment, int mtype)
{
	if (u->faction->no == MONSTER_FACTION) return;
	add_message(&u->faction->msgs, msg_message("mistake",
		"command error unit region", s, comment, u, u->region));
}

void
cmistake(const unit * u, const char *cmd, int mno, int mtype)
{
	static char ebuf[20];

	if (u->faction->no == MONSTER_FACTION) return;
	sprintf(ebuf, "error%d", mno);
	add_message(&u->faction->msgs, msg_message(ebuf, 
		"command unit region", cmd, u, u->region));
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
	return m;
}

void
free_messages(message_list * m)
{
	struct mlist * x = m->begin;
	while (x) {
		m->begin = x->next;
		msg_free(x->msg);
		free(x);
	}
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
