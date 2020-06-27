#include <platform.h>
#include "vortex.h"

#include <kernel/config.h>
#include <kernel/region.h>

#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/language.h>
#include <util/log.h>
#include <util/strings.h>
#include <util/umlaut.h>
#include <util/variant.h>

#include <storage.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>


typedef struct dir_lookup {
    char *name;
    const char *oldname;
    struct dir_lookup *next;
} dir_lookup;

static dir_lookup *dir_name_lookup;

void free_special_directions(void)
{
    while (dir_name_lookup) {
        dir_lookup *dl = dir_name_lookup;
        dir_name_lookup = dl->next;
        free(dl->name);
        free(dl);
    }
}

void register_special_direction(struct locale *lang, const char *name)
{
    const char *token = locale_string(lang, name, false);

    if (token) {
        void **tokens = get_translations(lang, UT_SPECDIR);
        variant var;
        char *str = str_strdup(name);

        var.v = str;
        addtoken((struct tnode **)tokens, token, var);

        if (lang == locales) {
            dir_lookup *dl = malloc(sizeof(dir_lookup));
            assert(dl);
            dl->name = str;
            dl->oldname = token;
            dl->next = dir_name_lookup;
            dir_name_lookup = dl;
        }
    }
    else {
        log_debug("no translation for spec_direction '%s' in locale '%s'\n", name, locale_name(lang));
    }
}

/********************/
/*   at_direction   */
/********************/
static void a_initdirection(variant * var)
{
    var->v = calloc(1, sizeof(spec_direction));
}

static void a_freedirection(variant *var)
{
    spec_direction *d = (spec_direction *)(var->v);
    free(d->desc);
    free(d->keyword);
    free(d);
}

static int a_agedirection(attrib * a, void *owner)
{
    spec_direction *d = (spec_direction *)(a->data.v);
    (void)owner;
    --d->duration;
    return (d->duration > 0) ? AT_AGE_KEEP : AT_AGE_REMOVE;
}

static int a_readdirection(variant *var, void *owner, struct gamedata *data)
{
    struct storage *store = data->store;
    spec_direction *d = (spec_direction *)(var->v);
    char lbuf[32]; /* Flawfinder: ignore */

    (void)owner;
    READ_INT(store, &d->x);
    READ_INT(store, &d->y);
    READ_INT(store, &d->duration);
    READ_TOK(store, lbuf, sizeof(lbuf));
    d->desc = str_strdup(lbuf);
    READ_TOK(store, lbuf, sizeof(lbuf));
    d->keyword = str_strdup(lbuf);
    d->active = true;
    return AT_READ_OK;
}

static void
a_writedirection(const variant *var, const void *owner, struct storage *store)
{
    spec_direction *d = (spec_direction *)(var->v);

    (void)owner;
    WRITE_INT(store, d->x);
    WRITE_INT(store, d->y);
    WRITE_INT(store, d->duration);
    WRITE_TOK(store, d->desc);
    WRITE_TOK(store, d->keyword);
}
attrib_type at_direction = {
    "direction",
    a_initdirection,
    a_freedirection,
    a_agedirection,
    a_writedirection,
    a_readdirection
};

region *find_special_direction(const region * r, const char *token)
{
    attrib *a;

    if (*token == '\0')
        return NULL;
    for (a = a_find(r->attribs, &at_direction); a && a->type == &at_direction;
        a = a->next) {
        spec_direction *d = (spec_direction *)(a->data.v);

        if (d->active && strcmp(token, d->keyword) == 0) {
            return findregion(d->x, d->y);
        }
    }

    return NULL;
}

attrib *create_special_direction(region * r, region * rt, int duration,
    const char *desc, const char *keyword, bool active)
{
    attrib *a = a_add(&r->attribs, a_new(&at_direction));
    spec_direction *d = (spec_direction *)(a->data.v);

    d->active = active;
    d->x = rt->x;
    d->y = rt->y;
    d->duration = duration;
    d->desc = str_strdup(desc);
    d->keyword = str_strdup(keyword);

    return a;
}

spec_direction *special_direction(const region * from, const region * to)
{
    const attrib *a = a_find(from->attribs, &at_direction);

    while (a != NULL && a->type == &at_direction) {
        spec_direction *sd = (spec_direction *)a->data.v;
        if (sd->x == to->x && sd->y == to->y)
            return sd;
        a = a->next;
    }
    return NULL;
}
