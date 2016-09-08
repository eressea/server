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

#ifndef EVENT_H
#define EVENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "variant.h"

    struct attrib;
    struct trigger;
    struct storage;
    struct gamedata;

    typedef struct trigger_type {
        const char *name;
        void(*initialize) (struct trigger *);
        void(*finalize) (struct trigger *);
        int(*handle) (struct trigger *, void *);
        void(*write) (const struct trigger *, struct storage * store);
        int(*read) (struct trigger *, struct gamedata * store);

        struct trigger_type *next;
    } trigger_type;

    trigger_type *tt_find(const char *name);
    void tt_register(trigger_type * ttype);

    typedef struct trigger {
        struct trigger_type *type;
        struct trigger *next;

        variant data;
    } trigger;

    typedef struct event_arg {
        const char *type;
        variant data;
    } event_arg;

    trigger *t_new(trigger_type * ttype);
    void t_free(trigger * t);
    void t_add(trigger ** tlist, trigger * t);
    /** add and handle triggers **/

    /* add a trigger to a list of attributes */
    void add_trigger(struct attrib **ap, const char *eventname,
    struct trigger *t);
    void remove_triggers(struct attrib **ap, const char *eventname,
        const trigger_type * tt);
    struct trigger **get_triggers(struct attrib *ap,
        const char *eventname);
    /* calls handle() for each of these. e.g. used in timeout */
    void handle_event(struct attrib *attribs, const char *eventname,
        void *data);

    /* functions for making complex triggers: */
    void free_triggers(trigger * triggers);        /* release all these triggers */
    void write_triggers(struct storage *store, const trigger * t);
    int read_triggers(struct gamedata *data, trigger ** tp);
    int handle_triggers(trigger ** triggers, void *data);

    extern struct attrib_type at_eventhandler;

#ifdef __cplusplus
}
#endif
#endif
