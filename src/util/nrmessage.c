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
#include "nrmessage_struct.h"

/* util includes */
#include "bsdstring.h"
#include "log.h"
#include "message.h"
#include "language.h"
#include "translation.h"
#include "strings.h"

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define NRT_MAXHASH 1021
static nrmessage_type *nrtypes[NRT_MAXHASH];

const char *nrt_string(const struct nrmessage_type *type)
{
    return type->string;
}

nrmessage_type *nrt_find(const struct locale * lang,
    const struct message_type * mtype)
{
    nrmessage_type *found = NULL;
    unsigned int hash = hashstring(mtype->name) % NRT_MAXHASH;
    nrmessage_type *type = nrtypes[hash];
    while (type) {
        if (type->mtype == mtype) {
            if (found == NULL)
                found = type;
            else if (type->lang == NULL)
                found = type;
            if (lang == type->lang) {
                found = type;
                break;
            }
        }
        type = type->next;
    }
    if (!found) {
        log_warning("could not find nr-type %s for locale %s\n", mtype->name, locale_name(lang));
    }
    if (lang && found && found->lang != lang) {
        log_warning("could not find nr-type %s for locale %s, using %s\n", mtype->name, locale_name(lang), locale_name(found->lang));
    }
    return found;
}

nrsection *sections;

const nrsection *section_find(const char *name)
{
    nrsection **mcp = &sections;
    if (name == NULL)
        return NULL;
    for (; *mcp; mcp = &(*mcp)->next) {
        nrsection *mc = *mcp;
        if (!strcmp(mc->name, name))
            break;
    }
    return *mcp;
}

const nrsection *section_add(const char *name)
{
    nrsection **mcp = &sections;
    if (name == NULL)
        return NULL;
    for (; *mcp; mcp = &(*mcp)->next) {
        nrsection *mc = *mcp;
        if (!strcmp(mc->name, name))
            break;
    }
    if (!*mcp) {
        nrsection *mc = calloc(sizeof(nrsection), 1);
        mc->name = strdup(name);
        *mcp = mc;
    }
    return *mcp;
}

void
nrt_register(const struct message_type *mtype, const struct locale *lang,
const char *string, int level, const char *section)
{
    unsigned int hash = hashstring(mtype->name) % NRT_MAXHASH;
    nrmessage_type *nrt = nrtypes[hash];
    while (nrt && (nrt->lang != lang || nrt->mtype != mtype)) {
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
        nrt->lang = lang;
        nrt->mtype = mtype;
        nrt->next = nrtypes[hash];
        nrt->level = level;
        if (section) {
            const nrsection *s = section_find(section);
            if (s == NULL) {
                s = section_add(section);
            }
            nrt->section = s->name;
        }
        else
            nrt->section = NULL;
        nrtypes[hash] = nrt;
        assert(string && *string);
        nrt->string = strdup(string);
        *c = '\0';
        for (i = 0; i != mtype->nparameters; ++i) {
            if (i != 0)
                *c++ = ' ';
            c += strlcpy(c, mtype->pnames[i], sizeof(zNames)-(c-zNames));
        }
        nrt->vars = strdup(zNames);
    }
}

size_t
nr_render(const struct message *msg, const struct locale *lang, char *buffer,
size_t size, const void *userdata)
{
    struct nrmessage_type *nrt = nrt_find(lang, msg->type);

    if (nrt) {
        const char *m =
            translate(nrt->string, userdata, nrt->vars, msg->parameters);
        if (m) {
            return strlcpy((char *)buffer, m, size);
        }
        else {
            log_error("Couldn't render message %s\n", nrt->mtype->name);
        }
    }
    if (size > 0 && buffer)
        buffer[0] = 0;
    return 0;
}

int nr_level(const struct message *msg)
{
    nrmessage_type *nrt = nrt_find(NULL, msg->type);
    return nrt ? nrt->level : 0;
}

const char *nr_section(const struct message *msg)
{
    nrmessage_type *nrt = nrt_find(default_locale, msg->type);
    return nrt ? nrt->section : NULL;
}

const char *nrt_section(const nrmessage_type * nrt)
{
    return nrt ? nrt->section : NULL;
}
