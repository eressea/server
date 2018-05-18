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
#include "nrmessage.h"

/* util includes */
#include "log.h"
#include "message.h"
#include "language.h"
#include "translation.h"
#include "strings.h"

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct nrmessage_type {
    const struct message_type *mtype;
    char *vars;
    struct nrmessage_type *next;
} nrmessage_type;

#define NRT_MAXHASH 1021
static nrmessage_type *nrtypes[NRT_MAXHASH];

const char *nrt_string(const struct message_type *mtype, 
        const struct locale *lang)
{
    const char * str = locale_getstring(lang, mtype->name);
    if (!str) {
        str = locale_getstring(default_locale, mtype->name);
    }
    assert(str);
    return str;
}

static nrmessage_type *nrt_find(const struct message_type * mtype)
{
    nrmessage_type *found = NULL;
    unsigned int hash = mtype->key % NRT_MAXHASH;
    nrmessage_type *type = nrtypes[hash];
    while (type) {
        if (type->mtype == mtype) {
            if (found == NULL) {
                found = type;
            }
        }
        type = type->next;
    }
    if (!found) {
        log_warning("could not find nr-type %s\n", mtype->name);
    }
    return found;
}

void
nrt_register(const struct message_type *mtype)
{
    unsigned int hash = mtype->key % NRT_MAXHASH;
    nrmessage_type *nrt = nrtypes[hash];
    while (nrt && nrt->mtype != mtype) {
        nrt = nrt->next;
    }
    if (nrt) {
        log_error("duplicate message-type %s\n", mtype->name);
        assert(!nrt || !"trying to register same nr-type twice");
    }
    else {
        int i;
        char zNames[256];
        char *c = zNames;
        nrt = malloc(sizeof(nrmessage_type));
        nrt->mtype = mtype;
        nrt->next = nrtypes[hash];
        nrtypes[hash] = nrt;
        *c = '\0';
        for (i = 0; i != mtype->nparameters; ++i) {
            if (i != 0)
                *c++ = ' ';
            c += str_strlcpy(c, mtype->pnames[i], sizeof(zNames)-(c-zNames));
        }
        nrt->vars = str_strdup(zNames);
    }
}

size_t
nr_render(const struct message *msg, const struct locale *lang, char *buffer,
size_t size, const void *userdata)
{
    struct nrmessage_type *nrt = nrt_find(msg->type);

    if (nrt) {
        const char *m =
            translate(nrt_string(msg->type, lang), userdata, nrt->vars, msg->parameters);
        if (m) {
            return str_strlcpy((char *)buffer, m, size);
        }
        else {
            log_error("Couldn't render message %s\n", nrt->mtype->name);
        }
    }
    if (size > 0 && buffer)
        buffer[0] = 0;
    return 0;
}

void free_nrmesssages(void) {
    int i;
    for (i = 0; i != NRT_MAXHASH; ++i) {
        while (nrtypes[i]) {
            nrmessage_type *nr = nrtypes[i];
            nrtypes[i] = nr->next;
            free(nr->vars);
            free(nr);
        }
    }
}

