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

#include <platform.h>
#include "message.h"

#include "strings.h"
#include "log.h"
#include "selist.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void(*msg_log_create) (const struct message * msg) = 0;

const char *mt_name(const message_type * mtype)
{
    return mtype->name;
}

static arg_type *argtypes = NULL;

void
register_argtype(const char *name, void(*free_arg) (variant),
variant(*copy_arg) (variant), variant_type type)
{
    arg_type *atype = (arg_type *)malloc(sizeof(arg_type));
    atype->name = name;
    atype->next = argtypes;
    atype->release = free_arg;
    atype->copy = copy_arg;
    atype->vtype = type;
    argtypes = atype;
}

arg_type *find_argtype(const char *name)
{
    arg_type *atype = argtypes;
    while (atype != NULL) {
        if (strcmp(atype->name, name) == 0)
            return atype;
        atype = atype->next;
    }
    return NULL;
}

message_type *mt_new(const char *name, const char *args[])
{
    int i, nparameters = 0;
    message_type *mtype = (message_type *)malloc(sizeof(message_type));

    assert(name != NULL);
    if (name == NULL) {
        log_error("Trying to create message_type with name=0x0\n");
        return NULL;
    }
    if (args != NULL) {
        /* count the number of parameters */
        while (args[nparameters]) ++nparameters;
    }
    mtype->key = 0;
    mtype->name = _strdup(name);
    mtype->nparameters = nparameters;
    if (nparameters > 0) {
        mtype->pnames = (char **)malloc(sizeof(char *) * nparameters);
        mtype->types = (arg_type **)malloc(sizeof(arg_type *) * nparameters);
    }
    else {
        mtype->pnames = NULL;
        mtype->types = NULL;
    }
    if (args != NULL) {
        for (i = 0; args[i]; ++i) {
            const char *x = args[i];
            const char *spos = strchr(x, ':');
            struct arg_type *atype = NULL;
            if (spos != NULL) {
                atype = find_argtype(spos + 1);
            }
            if (!atype) {
                log_error("unknown argument type %s for message type %s\n", spos + 1, mtype->name);
                assert(atype);
            }
            else {
                char *cp;
                cp = malloc(spos - x + 1);
                memcpy(cp, x, spos - x);
                cp[spos - x] = '\0';
                mtype->pnames[i] = cp;
                mtype->types[i] = atype;
            }
        }
    }
    return mtype;
}

message_type *mt_new_va(const char *name, ...)
{
    const char *args[16];
    int i = 0;
    va_list marker;

    va_start(marker, name);
    for (;;) {
        const char *c = va_arg(marker, const char *);
        args[i++] = c;
        if (c == NULL)
            break;
    }
    va_end(marker);
    return mt_new(name, args);
}

static variant copy_arg(const arg_type * atype, variant data)
{
    assert(atype != NULL);
    if (atype->copy == NULL)
        return data;
    return atype->copy(data);
}

static void free_arg(const arg_type * atype, variant data)
{
    assert(atype != NULL);
    if (atype->release)
        atype->release(data);
}

message *msg_create(const struct message_type *mtype, variant args[])
{
    int i;
    message *msg = (message *)malloc(sizeof(message));

    assert(mtype != NULL);
    if (mtype == NULL) {
        log_error("Trying to create message with type=0x0\n");
        return NULL;
    }
    msg->type = mtype;
    msg->parameters = (variant *)(mtype->nparameters ? calloc(mtype->nparameters, sizeof(variant)) : NULL);
    msg->refcount = 1;
    for (i = 0; i != mtype->nparameters; ++i) {
        msg->parameters[i] = copy_arg(mtype->types[i], args[i]);
    }
    if (msg_log_create)
        msg_log_create(msg);
    return msg;
}

#define MT_MAXHASH 1021
static selist *messagetypes[MT_MAXHASH];

static void mt_free(void *val) {
    message_type *mtype = (message_type *)val;
    int i;
    for (i = 0; i != mtype->nparameters; ++i) {
        free(mtype->pnames[i]);
    }
    free(mtype->pnames);
    free(mtype->types);
    free(mtype->name);
    free(mtype);
}

void mt_clear(void) {
    int i;
    for (i = 0; i != MT_MAXHASH; ++i) {
        selist *ql = messagetypes[i];
        selist_foreach(ql, mt_free);
        selist_free(ql);
        messagetypes[i] = 0;
    }
}

const message_type *mt_find(const char *name)
{
    unsigned int hash = hashstring(name) % MT_MAXHASH;
    selist *ql = messagetypes[hash];
    int qi;

    for (qi = 0; ql; selist_advance(&ql, &qi, 1)) {
        message_type *data = (message_type *)selist_get(ql, qi);
        if (strcmp(data->name, name) == 0) {
            return data;
        }
    }
    return 0;
}

static unsigned int mt_id(const message_type * mtype)
{
    unsigned int key = 0;
    size_t i = strlen(mtype->name);

    while (i > 0) {
        key = (mtype->name[--i] + key * 37);
    }
    return key % 0x7FFFFFFF;
}

const message_type *mt_register(message_type * type)
{
    unsigned int hash = hashstring(type->name) % MT_MAXHASH;
    selist **qlp = messagetypes + hash;

    if (selist_set_insert(qlp, type, NULL)) {
        type->key = mt_id(type);
    }
    return type;
}

void msg_free(message * msg)
{
    int i;
    assert(msg->refcount == 0);
    for (i = 0; i != msg->type->nparameters; ++i) {
        free_arg(msg->type->types[i], msg->parameters[i]);
    }
    free((void *)msg->parameters);
    free(msg);
}

void msg_release(struct message *msg)
{
    assert(msg->refcount > 0);
    if (--msg->refcount > 0)
        return;
    msg_free(msg);
}

struct message *msg_addref(struct message *msg)
{
    assert(msg->refcount > 0);
    ++msg->refcount;
    return msg;
}

void message_done(void) {
    arg_type **atp = &argtypes;
    while (*atp) {
        arg_type *at = *atp;
        *atp = at->next;
        free(at);
    }
}
