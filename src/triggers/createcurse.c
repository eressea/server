#include <platform.h>
#include "createcurse.h"

/* kernel includes */
#include <kernel/curse.h>
#include <kernel/unit.h>

/* util includes */
#include <kernel/attrib.h>
#include <util/base36.h>
#include <kernel/event.h>
#include <kernel/gamedata.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/resolve.h>

#include <storage.h>

/* ansi includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <stdio.h>
/***
 ** restore a mage that was turned into a toad
 **/

typedef struct createcurse_data {
    struct unit *mage;
    struct unit *target;
    const curse_type *type;
    double vigour;
    int duration;
    double effect;
    int men;
} createcurse_data;

static void createcurse_init(trigger * t)
{
    t->data.v = calloc(1, sizeof(createcurse_data));
}

static void createcurse_free(trigger * t)
{
    free(t->data.v);
}

static int createcurse_handle(trigger * t, void *data)
{
    /* call an event handler on createcurse.
     * data.v -> ( variant event, int timer )
     */
    createcurse_data *td = (createcurse_data *)t->data.v;
    if (td->mage && td->target && td->mage->number && td->target->number) {
        create_curse(td->mage, &td->target->attribs,
            td->type, td->vigour, td->duration, td->effect, td->men);
    }
    else {
        log_error("could not perform createcurse::handle()\n");
    }
    UNUSED_ARG(data);
    return 0;
}

static void createcurse_write(const trigger * t, struct storage *store)
{
    createcurse_data *td = (createcurse_data *)t->data.v;
    write_unit_reference(td->mage, store);
    write_unit_reference(td->target, store);
    WRITE_TOK(store, td->type->cname);
    WRITE_FLT(store, (float)td->vigour);
    WRITE_INT(store, td->duration);
    WRITE_FLT(store, (float)td->effect);
    WRITE_INT(store, td->men);
}

static int createcurse_read(trigger * t, gamedata *data)
{
    createcurse_data *td = (createcurse_data *)t->data.v;
    char zText[128];
    float flt;

    read_unit_reference(data, &td->mage, NULL);
    read_unit_reference(data, &td->target, NULL);

    READ_TOK(data->store, zText, sizeof(zText));
    td->type = ct_find(zText);
    READ_FLT(data->store, &flt);
    td->vigour = flt;
    READ_INT(data->store, &td->duration);
    if (data->version < CURSEFLOAT_VERSION) {
        int n;
        READ_INT(data->store, &n);
        td->effect = (float)n;
    }
    else {
        READ_FLT(data->store, &flt);
        td->effect = flt;
    }
    READ_INT(data->store, &td->men);
    return AT_READ_OK;
}

trigger_type tt_createcurse = {
    "createcurse",
    createcurse_init,
    createcurse_free,
    createcurse_handle,
    createcurse_write,
    createcurse_read
};

trigger *trigger_createcurse(struct unit * mage, struct unit * target,
    const curse_type * ct, double vigour, int duration, double effect, int men)
{
    trigger *t = t_new(&tt_createcurse);
    createcurse_data *td = (createcurse_data *)t->data.v;
    td->mage = mage;
    td->target = target;
    td->type = ct;
    td->vigour = vigour;
    td->duration = duration;
    td->effect = effect;
    td->men = men;
    return t;
}
