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
#include <util/macros.h>
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

#define MAXSTRLEN (4*DISPLAYSIZE+3)

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

static int missing_message_mode;

void message_handle_missing(int mode) {
    missing_message_mode = mode;
}

static message *missing_feedback(const char *name, const struct unit *u,
    const struct region *r, struct order *ord)
{
    if (missing_message_mode == MESSAGE_MISSING_ERROR) {
        log_error("trying to create undefined feedback of type \"%s\"\n", name);
    }
    else if (missing_message_mode == MESSAGE_MISSING_REPLACE) {
        log_warning("trying to create undefined message of type \"%s\"\n", name);
        if (strcmp(name, "missing_feedback") != 0) {
            if (!mt_find("missing_feedback")) {
                mt_create_va(mt_new("missing_feedback", NULL), "unit:unit",
                    "region:region", "command:order", "name:string", MT_NEW_END);
            }
            return msg_message("missing_feedback", "unit region command name", u, r, ord, name);
        }
    }
    return NULL;
}

static message *missing_message(const char *name) {
    if (missing_message_mode == MESSAGE_MISSING_ERROR) {
        log_error("trying to create undefined message of type \"%s\"\n", name);
    }
    else if (missing_message_mode == MESSAGE_MISSING_REPLACE) {
        log_warning("trying to create undefined message of type \"%s\"\n", name);
        if (strcmp(name, "missing_message") != 0) {
            if (!mt_find("missing_message")) {
                mt_create_va(mt_new("missing_message", NULL), "name:string", MT_NEW_END);
            }
            return msg_message("missing_message", "name", name);
        }
    }
    return NULL;
}

struct message *msg_feedback(const struct unit *u, struct order *ord,
    const char *name, const char *sig, ...)
{
    const message_type *mtype = mt_find(name);
    variant args[16];
    variant var;
    memset(args, 0, sizeof(args));

    if (ord && is_silent(ord)) {
        return NULL;
    }

    if (!mtype) {
        return missing_feedback(name, u, u->region, ord);
    }

    var.v = (void *)u;
    arg_set(args, mtype, "unit", var);
    var.v = (void *)u->region;
    arg_set(args, mtype, "region", var);
    var.v = (void *)ord;
    arg_set(args, mtype, "command", var);

    if (sig) {
        const char *ic = sig;
        va_list marker;

        va_start(marker, sig);
        while (*ic && !isalnum(*ic))
            ic++;
        while (*ic) {
            char paramname[64];
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
    }
    return msg_create(mtype, args);
}

message *msg_message(const char *name, const char *sig, ...)
/* msg_message("oops_error", "unit region command", u, r, cmd) */
{
    va_list vargs;
    const message_type *mtype = mt_find(name);
    char paramname[64];
    const char *ic = sig;
    int argnum=0;
    variant args[16];
    memset(args, 0, sizeof(args));

    if (!mtype) {
        return missing_message(name);
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
            argnum++;
        }
        else {
            log_error("invalid parameter %s for message type %s\n", paramname, mtype->name);
        }
        while (*ic && !isalnum(*ic))
            ic++;
    }
    va_end(vargs);
    if (argnum !=  mtype->nparameters) {
        log_error("not enough parameters for message type %s\n", mtype->name);
    }

    return msg_create(mtype, args);
}

static void
caddmessage(region * r, faction * f, const char *s, msg_t mtype, int level)
{
    message *m = NULL;

    UNUSED_ARG(level);
    switch (mtype) {
    case MSG_BATTLE:
        assert(0 || !"battle messages must not use addmessage");
        break;
    case MSG_MAGIC:
    case MSG_MESSAGE:
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

    if (u->faction->flags & FFL_NPC) {
        return NULL;
    }
    sprintf(msgname, "error%d", mno);
    return msg_feedback(u, ord, msgname, "");
}

void cmistake(const unit * u, struct order *ord, int mno, int mtype)
{
    message * msg;
    UNUSED_ARG(mtype);
    msg = msg_error(u, ord, mno);
    if (msg) {
        ADDMSG(&u->faction->msgs, msg);
    }
}

void syntax_error(const struct unit *u, struct order *ord)
{
    message * result;
    result = msg_error(u, ord, 10);
    if (result) {
        ADDMSG(&u->faction->msgs, result);
    }
}

void free_messagelist(mlist *msgs)
{
    struct mlist **mlistptr;
    for (mlistptr = &msgs; *mlistptr;) {
        struct mlist *ml = *mlistptr;
        *mlistptr = ml->next;
        msg_release(ml->msg);
        free(ml);
    }
}

message *add_message(message_list ** pm, message * m)
{
    assert(m && m->type);
    if (m != NULL) {
        struct mlist *mnew = malloc(sizeof(struct mlist));
        if (!mnew) abort();
        if (*pm == NULL) {
            *pm = malloc(sizeof(message_list));
            if (*pm == NULL) abort();
            (*pm)->end = &(*pm)->begin;
        }
        mnew->msg = msg_addref(m);
        mnew->next = NULL;
        *((*pm)->end) = mnew;
        (*pm)->end = &mnew->next;
    }
    return m;
}

struct mlist ** merge_messages(message_list *mlist, message_list *append) {
    struct mlist **split = 0;
    assert(mlist);
    if (append) {
        split = mlist->end;
        *split = append->begin;
        mlist->end = append->end;
    }
    return split;
}

void split_messages(message_list *mlist, struct mlist **split) {
    assert(mlist);
    if (split) {
        *split = 0;
        mlist->end = split;
    }
}
