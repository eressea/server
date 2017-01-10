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
#include "crmessage.h"

#include "message.h"
#include "strings.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/** type to string conversion **/
typedef struct tsf_list {
    struct tsf_list *next;
    const char *name;
    tostring_f fun;
} tsf_list;

static tsf_list *tostringfs;

void crmessage_done(void) {
    tsf_list **tsp = &tostringfs;
    while (*tsp) {
        tsf_list *ts = *tsp;
        *tsp = ts->next;
        free(ts);
    }
}

static tostring_f tsf_find(const char *name)
{
    if (name != NULL) {
        tsf_list *tsf;
        for (tsf = tostringfs; tsf; tsf = tsf->next) {
            if (!strcmp(tsf->name, name))
                return tsf->fun;
        }
    }
    return NULL;
}

void tsf_register(const char *name, tostring_f fun)
{
    tsf_list *tsf;
    for (tsf = tostringfs; tsf; tsf = tsf->next) {
        if (!strcmp(tsf->name, name))
            break;
    }
    if (tsf == NULL) {
        tsf = malloc(sizeof(tsf_list));
        tsf->fun = fun;
        tsf->name = name;
        tsf->next = tostringfs;
        tostringfs = tsf;
    }
}

/** crmesssage **/
typedef struct crmessage_type {
    const struct message_type *mtype;
    tostring_f *renderers;
    struct crmessage_type *next;
} crmessage_type;

#define CRMAXHASH 63
static crmessage_type *crtypes[CRMAXHASH];

static crmessage_type *crt_find(const struct message_type *mtype)
{
    unsigned int hash = hashstring(mtype->name) % CRMAXHASH;
    crmessage_type *found = NULL;
    crmessage_type *type = crtypes[hash];
    while (type) {
        if (type->mtype == mtype)
            found = type;
        type = type->next;
    }
    return found;
}

void crt_register(const struct message_type *mtype)
{
    unsigned int hash = hashstring(mtype->name) % CRMAXHASH;
    crmessage_type *crt = crtypes[hash];
    while (crt && crt->mtype != mtype) {
        crt = crt->next;
    }
    if (!crt) {
        crt = malloc(sizeof(crmessage_type));
        crt->mtype = mtype;
        crt->next = crtypes[hash];
        crtypes[hash] = crt;
        if (mtype->nparameters > 0) {
            int i;
            crt->renderers = malloc(sizeof(tostring_f) * mtype->nparameters);
            /* can be scrapped for memory vs. speed */
            for (i = 0; i != mtype->nparameters; ++i) {
                crt->renderers[i] = tsf_find(mtype->types[i]->name);
            }
        }
        else {
            crt->renderers = NULL;
        }
    }
}

int cr_render(const message * msg, char *buffer, const void *userdata)
{
    int i;
    char *c = buffer;
    struct crmessage_type *crt = crt_find(msg->type);

    if (crt == NULL)
        return -1;
    for (i = 0; i != msg->type->nparameters; ++i) {
        if (crt->renderers[i] == NULL) {
            log_error("No renderer for argument %s:%s of \"%s\"\n", msg->type->pnames[i], msg->type->types[i]->name, msg->type->name);
            continue;                 /* strcpy(c, (const char*)msg->locale_string(u->faction->locale, parameters[i])); */
        }
        else {
            if (crt->renderers[i](msg->parameters[i], c, userdata) != 0)
                continue;
        }
        c += strlen(c);
        sprintf(c, ";%s\n", msg->type->pnames[i]);
        c += strlen(c);
    }
    return 0;
}

int cr_string(variant var, char *buffer, const void *userdata)
{
    sprintf(buffer, "\"%s\"", (const char *)var.v);
    UNUSED_ARG(userdata);
    return 0;
}

int cr_int(variant var, char *buffer, const void *userdata)
{
    sprintf(buffer, "%d", var.i);
    UNUSED_ARG(userdata);
    return 0;
}

int cr_ignore(variant var, char *buffer, const void *userdata)
{
    UNUSED_ARG(var);
    UNUSED_ARG(buffer);
    UNUSED_ARG(userdata);
    return -1;
}
