#ifndef EVENT_H
#define EVENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <util/variant.h>

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
