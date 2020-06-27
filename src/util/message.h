#ifndef H_MESSAGE_H
#define H_MESSAGE_H

#include "variant.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MSG_MAXARGS 8
#define MT_NEW_END ((const char *)0)
#define MAXSECTIONS 16

    extern char *sections[MAXSECTIONS];

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
        const char *section;
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

    struct message *msg_create(const struct message_type *type,
        variant args[]);
    /* msg_create(&mt_simplesentence, "enno", "eats", "chocolate", &locale_de);
     * parameters must be in the same order as they were for mt_new! */

    void msg_release(struct message *msg);
    struct message *msg_addref(struct message *msg);

    const char *mt_name(const struct message_type *mtype);

    struct message_type *mt_new(const char *name, const char *section);
    /** message_type registry (optional): **/
    struct message_type *mt_create(struct message_type *, const char *args[], int nargs);
    struct message_type *mt_create_feedback(const char *name);
    struct message_type *mt_create_error(int error);
    struct message_type *mt_create_va(struct message_type *, ...);
    const struct message_type *mt_find(const char *);

    void register_argtype(const char *name, void(*free_arg) (variant),
        variant(*copy_arg) (variant), variant_type);
    arg_type *find_argtype(const char *name);

    void(*msg_log_create) (const struct message * msg);

#ifdef __cplusplus
}
#endif
#endif

