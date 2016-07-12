/* 
+-------------------+  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
+-------------------+
This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>
#include "unitmessage.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/messages.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/resolve.h>

#include <storage.h>

/* ansi includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/***
** give an item to someone
**/

typedef struct unitmessage_data {
    struct unit *target;
    char *string;
    int type;
    int level;
} unitmessage_data;

static void unitmessage_init(trigger * t)
{
    t->data.v = calloc(sizeof(unitmessage_data), 1);
}

static void unitmessage_free(trigger * t)
{
    unitmessage_data *sd = (unitmessage_data *)t->data.v;
    free(sd->string);
    free(t->data.v);
}

static int unitmessage_handle(trigger * t, void *data)
{
    /* call an event handler on unitmessage.
     * data.v -> ( variant event, int timer )
     */
    unitmessage_data *td = (unitmessage_data *)t->data.v;
    if (td->target && td->target->no) {
        struct faction *f = td->target->faction;
        const char * str = LOC(f->locale, td->string);
        /* bug found in turn 733: sometimes, alps have f*cked up messages */
        if (td->string && td->string[0]) {
            addmessage(td->target->region, f, str, td->type,
                td->level);
        }
    }
    unused_arg(data);
    return 0;
}

static void unitmessage_write(const trigger * t, struct storage *store)
{
    unitmessage_data *td = (unitmessage_data *)t->data.v;
    write_unit_reference(td->target, store);
    WRITE_TOK(store, td->string);
    WRITE_INT(store, td->type);
    WRITE_INT(store, td->level);
}

static int unitmessage_read(trigger * t, gamedata *data)
{
    unitmessage_data *td = (unitmessage_data *)t->data.v;
    char zText[256];

    int result = read_reference(&td->target, data, read_unit_reference,
        resolve_unit);
    READ_TOK(data->store, zText, sizeof(zText));
    td->string = _strdup(zText);
    READ_INT(data->store, &td->type);
    READ_INT(data->store, &td->level);

    if (result == 0 && td->target == NULL) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

trigger_type tt_unitmessage = {
    "unitmessage",
    unitmessage_init,
    unitmessage_free,
    unitmessage_handle,
    unitmessage_write,
    unitmessage_read
};

trigger *trigger_unitmessage(unit * target, const char *string, int type,
    int level)
{
    trigger *t = t_new(&tt_unitmessage);
    unitmessage_data *td = (unitmessage_data *)t->data.v;
    td->target = target;
    td->string = _strdup(string);
    td->type = type;
    td->level = level;
    return t;
}
