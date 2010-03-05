/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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

#include <platform.h>
#include <kernel/config.h>
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
#include <util/language.h>
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
#include <kernel/config.h>

static void
arg_set(variant args[], const message_type * mtype, const char * buffer, variant v)
{
	int i;
	for (i=0;i!=mtype->nparameters;++i) {
		if (!strcmp(buffer, mtype->pnames[i])) break;
	}
  if (i!=mtype->nparameters) {
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
  char paramname[64];
  const char *ic = sig;
  variant args[16];
  variant var;
  memset(args, 0, sizeof(args));

  if (ord==NULL) ord = u->thisorder;

  if (!mtype) {
    log_error(("trying to create message of unknown type \"%s\"\n", name));
    return msg_message("missing_feedback", "unit region command name", u, u->region, ord, name);
  }

  var.v = (void*)u;
  arg_set(args, mtype, "unit", var);
  var.v = (void*)u->region;
  arg_set(args, mtype, "region", var);
  var.v = (void*)ord;
  arg_set(args, mtype, "command", var);

  va_start(marker, sig);
  while (*ic && !isalnum(*ic)) ic++;
  while (*ic) {
    char * oc = paramname;
    int i;

    while (isalnum(*ic)) *oc++ = *ic++;
    *oc = '\0';

    for (i=0;i!=mtype->nparameters;++i) {
      if (!strcmp(paramname, mtype->pnames[i])) break;
    }
    if (i!=mtype->nparameters) {
      if (mtype->types[i]->vtype==VAR_VOIDPTR) {
        args[i].v = va_arg(marker, void*);
      } else if (mtype->types[i]->vtype==VAR_INT) {
        args[i].i = va_arg(marker, int);
      } else {
        assert(!"unknown variant type");
      }
    } else {
      log_error(("invalid parameter %s for message type %s\n", paramname, mtype->name));
      assert(!"program aborted.");
    }
    while (*ic && !isalnum(*ic)) ic++;
  }
  va_end(marker);

  return msg_create(mtype, args);
}

message * 
msg_message(const char * name, const char* sig, ...)
	/* msg_message("oops_error", "unit region command", u, r, cmd) */
{
  va_list marker;
  const message_type * mtype = mt_find(name);
  char paramname[64];
  const char *ic = sig;
  variant args[16];
  memset(args, 0, sizeof(args));
  
  if (!mtype) {
    log_error(("trying to create message of unknown type \"%s\"\n", name));
    return msg_message("missing_message", "name", name);
  }
  
  va_start(marker, sig);
  while (*ic && !isalnum(*ic)) ic++;
  while (*ic) {
    char * oc = paramname;
    int i;
    
    while (isalnum(*ic)) *oc++ = *ic++;
    *oc = '\0';
    
    for (i=0;i!=mtype->nparameters;++i) {
      if (!strcmp(paramname, mtype->pnames[i])) break;
    }
    if (i!=mtype->nparameters) {
      if (mtype->types[i]->vtype==VAR_VOIDPTR) {
        args[i].v = va_arg(marker, void*);
      } else if (mtype->types[i]->vtype==VAR_INT) {
        args[i].i = va_arg(marker, int);
      } else {
        assert(!"unknown variant type");
      }
    } else {
      log_error(("invalid parameter %s for message type %s\n", paramname, mtype->name));
      assert(!"program aborted.");
    }
    while (*ic && !isalnum(*ic)) ic++;
  }
  va_end(marker);

  return msg_create(mtype, args);
}

static void
caddmessage(region * r, faction * f, const char *s, msg_t mtype, int level)
{
  message * m = NULL;

#define LOG_ENGLISH
#ifdef LOG_ENGLISH
  if (f && f->locale!=default_locale) {
    log_warning(("message for locale \"%s\": %s\n", locale_name(f->locale), s));
  }
#endif
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
  caddmessage(r, f, s, mtype, level);
}

void
cmistake(const unit * u, struct order *ord, int mno, int mtype)
{
  static char msgname[20];
  unused(mtype);

  if (is_monsters(u->faction)) return;
  sprintf(msgname, "error%d", mno);
  ADDMSG(&u->faction->msgs, msg_feedback(u, ord, msgname, ""));
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


