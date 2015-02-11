/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */
#ifndef H_MESSAGE_H
#define H_MESSAGE_H

#include "variant.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct arg_type {
        struct arg_type *next;
        variant_type vtype;
        const char *name;
        void(*release) (variant);
        variant(*copy) (variant);
    } arg_type;

    typedef struct message_type {
        unsigned int key;
        const char *name;
        int nparameters;
        const char **pnames;
        const struct arg_type **types;
    } message_type;

    typedef struct message {
        const struct message_type *type;
        variant *parameters;
        int refcount;
    } message;

    extern struct message_type *mt_new(const char *name, const char **args);
    extern struct message_type *mt_new_va(const char *name, ...);
    /* mt_new("simple_sentence", "subject:string", "predicate:string",
     *        "object:string", "lang:locale", NULL); */

    extern struct message *msg_create(const struct message_type *type,
        variant args[]);
    /* msg_create(&mt_simplesentence, "enno", "eats", "chocolate", &locale_de);
     * parameters must be in the same order as they were for mt_new! */

    extern void msg_release(struct message *msg);
    extern struct message *msg_addref(struct message *msg);

    extern const char *mt_name(const struct message_type *mtype);

    /** message_type registry (optional): **/
    extern const struct message_type *mt_register(struct message_type *);
    extern const struct message_type *mt_find(const char *);

    extern void register_argtype(const char *name, void(*free_arg) (variant),
        variant(*copy_arg) (variant), variant_type);
    extern const struct arg_type *find_argtype(const char *name);

    extern void(*msg_log_create) (const struct message * msg);

#ifdef __cplusplus
}
#endif
#endif

