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
        char *name;
        int nparameters;
        char **pnames;
        struct arg_type ** types;
    } message_type;

    typedef struct message {
        const struct message_type *type;
        variant *parameters;
        int refcount;
    } message;

    void message_done(void);

    void mt_clear(void);
    struct message_type *mt_new(const char *name, const char *args[]);
    struct message_type *mt_new_va(const char *name, ...);
    /* mt_new("simple_sentence", "subject:string", "predicate:string",
     *        "object:string", "lang:locale", NULL); */

    struct message *msg_create(const struct message_type *type,
        variant args[]);
    /* msg_create(&mt_simplesentence, "enno", "eats", "chocolate", &locale_de);
     * parameters must be in the same order as they were for mt_new! */

    void msg_release(struct message *msg);
    struct message *msg_addref(struct message *msg);

    const char *mt_name(const struct message_type *mtype);

    /** message_type registry (optional): **/
    const struct message_type *mt_register(struct message_type *);
    const struct message_type *mt_find(const char *);

    void register_argtype(const char *name, void(*free_arg) (variant),
        variant(*copy_arg) (variant), variant_type);
    arg_type *find_argtype(const char *name);

    void(*msg_log_create) (const struct message * msg);

#ifdef __cplusplus
}
#endif
#endif

