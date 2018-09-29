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
#include "event.h"
#include "attrib.h"

/* util includes */
#include <kernel/gamedata.h>
#include <util/log.h>
#include <util/strings.h>

#include <storage.h>

/* libc includes */
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void write_triggers(struct storage *store, const trigger * t)
{
    while (t) {
        if (t->type->write) {
            WRITE_TOK(store, t->type->name);
            t->type->write(t, store);
        }
        t = t->next;
    }
    WRITE_TOK(store, "end");
}

int read_triggers(struct gamedata *data, trigger ** tp)
{
    for (;;) {
        trigger_type *ttype;
        char zText[128];

        READ_TOK(data->store, zText, sizeof(zText));
        if (!strcmp(zText, "end"))
            break;
        ttype = tt_find(zText);
        assert(ttype || !"unknown trigger-type");
        *tp = t_new(ttype);
        if (ttype->read) {
            int i = ttype->read(*tp, data);
            switch (i) {
            case AT_READ_OK:
                tp = &(*tp)->next;
                break;
            case AT_READ_FAIL:
                t_free(*tp);
                free(*tp);
                *tp = NULL;
                break;
            default:
                assert(!"invalid return value");
                break;
            }
        }
    }
    return 0;
}

trigger *t_new(trigger_type * ttype)
{
    trigger *t = calloc(sizeof(trigger), 1);
    t->type = ttype;
    if (ttype->initialize)
        ttype->initialize(t);
    return t;
}

void t_free(trigger * t)
{
    if (t->type->finalize)
        t->type->finalize(t);
}

void free_triggers(trigger * triggers)
{
    while (triggers) {
        trigger *t = triggers;
        triggers = t->next;
        t_free(t);
        free(t);
    }
}

int handle_triggers(trigger ** triggers, void *param)
{
    trigger **tp = triggers;
    while (*tp) {
        trigger *t = *tp;
        if (t->type->handle(t, param) != 0) {
            *tp = t->next;
            t_free(t);
            free(t);
        }
        else
            tp = &t->next;
    }
    return (*triggers != NULL);
}

/***
 ** at_eventhandler
 **/

typedef struct handler_info {
    char *event;
    trigger *triggers;
} handler_info;

static void init_handler(variant *var)
{
    var->v = calloc(sizeof(handler_info), 1);
}

static void free_handler(variant *var)
{
    handler_info *hi = (handler_info *)var->v;
    free_triggers(hi->triggers);
    free(hi->event);
    free(hi);
}

static void
write_handler(const variant *var, const void *owner, struct storage *store)
{
    handler_info *hi = (handler_info *)var->v;
    WRITE_TOK(store, hi->event);
    write_triggers(store, hi->triggers);
}

static int read_handler(variant *var, void *owner, gamedata *data)
{
    struct storage *store = data->store;
    char zText[128];
    handler_info *hi = (handler_info *)var->v;

    READ_TOK(store, zText, sizeof(zText));
    hi->event = str_strdup(zText);
    read_triggers(data, &hi->triggers);
    if (hi->triggers != NULL) {
        return AT_READ_OK;
    }
    return AT_READ_FAIL;
}

attrib_type at_eventhandler = {
    "eventhandler",
    init_handler,
    free_handler,
    NULL,
    write_handler,
    read_handler
};

struct trigger **get_triggers(struct attrib *ap, const char *eventname)
{
    handler_info *td = NULL;
    attrib *a = a_find(ap, &at_eventhandler);
    while (a != NULL && a->type == &at_eventhandler) {
        td = (handler_info *)a->data.v;
        if (strcmp(td->event, eventname) == 0) {
            return &td->triggers;
        }
        a = a->next;
    }
    return NULL;
}

void add_trigger(struct attrib **ap, const char *eventname, struct trigger *t)
{
    trigger **tp;
    handler_info *td = NULL;
    attrib *a = a_find(*ap, &at_eventhandler);
    assert(t->next == NULL);
    while (a != NULL && a->type == &at_eventhandler) {
        td = (handler_info *)a->data.v;
        if (!strcmp(td->event, eventname)) {
            break;
        }
        a = a->next;
    }
    if (a == NULL || a->type != &at_eventhandler) {
        a = a_add(ap, a_new(&at_eventhandler));
        td = (handler_info *)a->data.v;
        td->event = str_strdup(eventname);
    }
    tp = &td->triggers;
    while (*tp)
        tp = &(*tp)->next;
    *tp = t;
}

void handle_event(attrib * attribs, const char *eventname, void *data)
{
    while (attribs) {
        if (attribs->type == &at_eventhandler)
            break;
        attribs = attribs->nexttype;
    }
    while (attribs && attribs->type == &at_eventhandler) {
        handler_info *tl = (handler_info *)attribs->data.v;
        if (!strcmp(tl->event, eventname)) {
            handler_info *tl = (handler_info *)attribs->data.v;
            handle_triggers(&tl->triggers, data);
            break;
        }
        attribs = attribs->next;
    }
}

void t_add(struct trigger **tlist, struct trigger *t)
{
    while (*tlist)
        tlist = &(*tlist)->next;
    *tlist = t;
}

static trigger_type *triggertypes;

void tt_register(trigger_type * tt)
{
    tt->next = triggertypes;
    triggertypes = tt;
}

trigger_type *tt_find(const char *name)
{
    trigger_type *tt = triggertypes;
    while (tt && strcmp(tt->name, name)!=0)
        tt = tt->next;
    return tt;
}

void
remove_triggers(struct attrib **ap, const char *eventname,
const trigger_type * tt)
{
    trigger **tp = get_triggers(*ap, eventname);
    if (tp == NULL)
        return;
    while (*tp) {
        /* first, remove all gate-triggers */
        trigger *t = *tp;
        if (t->type == tt) {
            *tp = t->next;
            t_free(t);
            free(t);
        }
        else
            tp = &t->next;
    }
}

/***
 ** default events
 **/
