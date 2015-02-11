/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include "messages.h"

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
arg_set(variant args[], const message_type * mtype, const char *buffer,
variant v)
{
    int i;
    for (i = 0; i != mtype->nparameters; ++i) {
        if (!strcmp(buffer, mtype->pnames[i]))
            break;
    }
    if (i != mtype->nparameters) {
        args[i] = v;
    }
    else {
        log_error("invalid parameter %s for message type %s\n", buffer,
            mtype->name);
        assert(!"program aborted.");
    }
}

struct message *msg_feedback(const struct unit *u, struct order *ord,
    const char *name, const char *sig, ...)
{
    va_list marker;
    const message_type *mtype = mt_find(name);
    char paramname[64];
    const char *ic = sig;
    variant args[16];
    variant var;
    memset(args, 0, sizeof(args));

    if (ord == NULL)
        ord = u->thisorder;

    if (!mtype) {
        log_error("trying to create message of unknown type \"%s\"\n", name);
        return msg_message("missing_feedback", "unit region command name", u,
            u->region, ord, name);
    }

    var.v = (void *)u;
    arg_set(args, mtype, "unit", var);
    var.v = (void *)u->region;
    arg_set(args, mtype, "region", var);
    var.v = (void *)ord;
    arg_set(args, mtype, "command", var);

    va_start(marker, sig);
    while (*ic && !isalnum(*ic))
        ic++;
    while (*ic) {
        char *oc = paramname;
        int i;

        while (isalnum(*ic))
            *oc++ = *ic++;
        *oc = '\0';

        for (i = 0; i != mtype->nparameters; ++i) {
            if (!strcmp(paramname, mtype->pnames[i]))
                break;
        }
        if (i != mtype->nparameters) {
            if (mtype->types[i]->vtype == VAR_VOIDPTR) {
                args[i].v = va_arg(marker, void *);
            }
            else if (mtype->types[i]->vtype == VAR_INT) {
                args[i].i = va_arg(marker, int);
            }
            else {
                assert(!"unknown variant type");
            }
        }
        else {
            log_error("invalid parameter %s for message type %s\n", paramname, mtype->name);
            assert(!"program aborted.");
        }
        while (*ic && !isalnum(*ic))
            ic++;
    }
    va_end(marker);

    return msg_create(mtype, args);
}

message *msg_message(const char *name, const char *sig, ...)
/* msg_message("oops_error", "unit region command", u, r, cmd) */
{
    va_list vargs;
    const message_type *mtype = mt_find(name);
    char paramname[64];
    const char *ic = sig;
    variant args[16];
    memset(args, 0, sizeof(args));

    if (!mtype) {
        log_warning("trying to create message of unknown type \"%s\"\n", name);
        if (strcmp(name, "missing_message") != 0) {
            return msg_message("missing_message", "name", name);
        }
        return NULL;
    }

    va_start(vargs, sig);
    while (*ic && !isalnum(*ic))
        ic++;
    while (*ic) {
        char *oc = paramname;
        int i;

        while (isalnum(*ic))
            *oc++ = *ic++;
        *oc = '\0';

        for (i = 0; i != mtype->nparameters; ++i) {
            if (!strcmp(paramname, mtype->pnames[i]))
                break;
        }
        if (i != mtype->nparameters) {
            if (mtype->types[i]->vtype == VAR_VOIDPTR) {
                args[i].v = va_arg(vargs, void *);
            }
            else if (mtype->types[i]->vtype == VAR_INT) {
                args[i].i = va_arg(vargs, int);
            }
            else {
                assert(!"unknown variant type");
            }
        }
        else {
            log_error("invalid parameter %s for message type %s\n", paramname, mtype->name);
            assert(!"program aborted.");
        }
        while (*ic && !isalnum(*ic))
            ic++;
    }
    va_end(vargs);

    return msg_create(mtype, args);
}

static void
caddmessage(region * r, faction * f, const char *s, msg_t mtype, int level)
{
    message *m = NULL;

#define LOG_ENGLISH
#ifdef LOG_ENGLISH
    if (f && f->locale != default_locale) {
        log_warning("message for locale \"%s\": %s\n", locale_name(f->locale), s);
    }
#endif
    unused_arg(level);
    switch (mtype) {
    case MSG_INCOME:
        assert(f);
        m = add_message(&f->msgs, msg_message("msg_economy", "string", s));
        break;
    case MSG_BATTLE:
        assert(0 || !"battle messages must not use addmessage");
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
        }
        else {
            if (f == NULL)
                add_message(&r->msgs, m);
            else
                r_addmessage(r, f, m);
        }
        break;
    default:
        assert(!"invalid message class");
    }
    if (m)
        msg_release(m);
}

void addmessage(region * r, faction * f, const char *s, msg_t mtype, int level)
{
    caddmessage(r, f, s, mtype, level);
}

message * msg_error(const unit * u, struct order *ord, int mno) {
    static char msgname[20];

    if (fval(u->faction, FFL_NPC))
        return 0;
    sprintf(msgname, "error%d", mno);
    return msg_feedback(u, ord, msgname, "");
}

message * cmistake(const unit * u, struct order *ord, int mno, int mtype)
{
    message * result;
    unused_arg(mtype);
    result = msg_error(u, ord, mno);
    ADDMSG(&u->faction->msgs, result);
    return result;
}

extern unsigned int new_hashstring(const char *s);

void free_messagelist(message_list * msgs)
{
    struct mlist **mlistptr = &msgs->begin;
    while (*mlistptr) {
        struct mlist *ml = *mlistptr;
        *mlistptr = ml->next;
        msg_release(ml->msg);
        free(ml);
    }
    free(msgs);
}

message *add_message(message_list ** pm, message * m)
{
    if (!lomem && m != NULL) {
        struct mlist *mnew = malloc(sizeof(struct mlist));
        if (*pm == NULL) {
            *pm = malloc(sizeof(message_list));
            (*pm)->end = &(*pm)->begin;
        }
        mnew->msg = msg_addref(m);
        mnew->next = NULL;
        *((*pm)->end) = mnew;
        (*pm)->end = &mnew->next;
    }
    return m;
}
