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

#ifdef _MSC_VER
#include <platform.h>
#endif
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

static unsigned int mt_id(const message_type * mtype)
{
    unsigned int key = 0;
    size_t i = strlen(mtype->name);

    while (i > 0) {
        /* TODO: why not use str_hash? */
        key = (mtype->name[--i] + key * 37);
    }
    return key % 0x7FFFFFFF;
}

#define MT_MAXHASH 1021
static selist *messagetypes[MT_MAXHASH];

static void mt_register(message_type * mtype) {
    unsigned int hash = str_hash(mtype->name) % MT_MAXHASH;
    selist **qlp = messagetypes + hash;

    if (selist_set_insert(qlp, mtype, NULL)) {
        mtype->key = mt_id(mtype);
    }
}

message_type *mt_create(message_type * mtype, const char *args[], int nparameters)
{
    if (args != NULL && args[nparameters]) {
        /* count the number of parameters */
        do {
            ++nparameters;
        } while (args[nparameters]);
    }
    if (nparameters > 0) {
        int i;
        mtype->nparameters = nparameters;
        mtype->pnames = (char **)malloc(sizeof(char *) * nparameters);
        mtype->types = (arg_type **)malloc(sizeof(arg_type *) * nparameters);
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
    mt_register(mtype);
    return mtype;
}

char *sections[MAXSECTIONS];

const char *section_find(const char *name)
{
    int i;

    if (name == NULL) {
        return NULL;
    }

    for (i = 0; i != MAXSECTIONS && sections[i]; ++i) {
        if (strcmp(sections[i], name) == 0) {
            return sections[i];
        }
    }
    return NULL;
}

const char *section_add(const char *name) {
    int i;
    if (name == NULL) {
        return NULL;
    }
    for (i = 0; i != MAXSECTIONS && sections[i]; ++i) {
        if (strcmp(sections[i], name) == 0) {
            return sections[i];
        }
    }
    assert(i < MAXSECTIONS);
    assert(sections[i] == NULL);
    if (i + 1 < MAXSECTIONS) {
        sections[i + 1] = NULL;
    }
    return sections[i] = str_strdup(name);
}

message_type *mt_new(const char *name, const char *section)
{
    message_type *mtype;

    assert(name != NULL);
    if (name == NULL) {
        log_error("Trying to create message_type with name=0x0\n");
        return NULL;
    }
    mtype = (message_type *)malloc(sizeof(message_type));
    mtype->key = 0;
    mtype->name = str_strdup(name);
    mtype->section = section_find(section);
    if (!mtype->section) {
        mtype->section = section_add(section);
    }
    mtype->nparameters = 0;
    mtype->pnames = NULL;
    mtype->types = NULL;
    return mtype;
}

message_type *mt_create_va(message_type *mtype, ...)
{
    const char *args[16];
    int i = 0;
    va_list marker;

    va_start(marker, mtype);
    for (;;) {
        const char *c = va_arg(marker, const char *);
        args[i++] = c;
        if (c == NULL)
            break;
    }
    va_end(marker);
    args[i] = NULL;
    return mt_create(mtype, args, i - 1);
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
    message *msg;

    assert(mtype != NULL);
    if (mtype == NULL) {
        log_error("Trying to create message with type=0x0\n");
        return NULL;
    }
    msg = (message *)malloc(sizeof(message));
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
    for (i = 0; i != MAXSECTIONS && sections[i]; ++i) {
        free(sections[i]);
        sections[i] = NULL;
    }
}

const message_type *mt_find(const char *name)
{
    unsigned int hash = str_hash(name) % MT_MAXHASH;
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
    assert(msg && msg->refcount > 0);
    if (--msg->refcount > 0)
        return;
    msg_free(msg);
}

struct message *msg_addref(struct message *msg)
{
    assert(msg && msg->refcount > 0);
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

