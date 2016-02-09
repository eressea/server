/*
 *
 * Eressea PB(E)M host Copyright (C) 1998-2015
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <platform.h>
#include <kernel/config.h>
#include "alp.h"

#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/resolve.h>
#include <util/umlaut.h>

#include "monster.h"

#include <storage.h>

#include <triggers/createcurse.h>
#include <triggers/killunit.h>
#include <triggers/removecurse.h>
#include <triggers/unitmessage.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

extern const char *directions[];

typedef struct alp_data {
    unit *mage;
    unit *target; // TODO: remove, use attribute-owner?
} alp_data;

static void alp_init(attrib * a)
{
    a->data.v = calloc(sizeof(alp_data), 1);
}

static void alp_done(attrib * a)
{
    free(a->data.v);
}

static int alp_verify(attrib * a, void *owner)
{
    alp_data *ad = (alp_data *)a->data.v;
    unused_arg(owner);
    if (ad->mage && ad->target)
        return 1;
    return 0;                     /* remove the attribute */
}

static void
alp_write(const attrib * a, const void *owner, struct storage *store)
{
    alp_data *ad = (alp_data *)a->data.v;
    write_unit_reference(ad->mage, store);
    write_unit_reference(ad->target, store);
}

static int alp_read(attrib * a, void *owner, struct storage *store)
{
    alp_data *ad = (alp_data *)a->data.v;
    int rm = read_reference(&ad->mage, store, read_unit_reference, resolve_unit);
    int rt =
        read_reference(&ad->target, store, read_unit_reference, resolve_unit);
    if (rt == 0 && rm == 0 && (!ad->target || !ad->mage)) {
        /* the target or mage disappeared. */
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

static attrib_type at_alp = {
    "alp",
    alp_init,
    alp_done,
    alp_verify,
    alp_write,
    alp_read,
    NULL,
    ATF_UNIQUE
};

int sp_summon_alp(struct castorder *co)
{
    unit *alp, *opfer;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;
    const struct race *rc = get_race(RC_ALP);
    struct faction *f = get_monsters();
    struct message *msg;

    opfer = pa->param[0]->data.u;

    /* Der Alp gehört den Monstern, darum erhält der Magier auch keine
     * Regionsberichte von ihm.  Er erhält aber später eine Mitteilung,
     * sobald der Alp sein Opfer erreicht hat.
     */
    alp = create_unit(r, f, 1, rc, 0, NULL, NULL);
    set_level(alp, SK_STEALTH, 7);
    setstatus(alp, ST_FLEE);      /* flieht */

    {
        attrib *a = a_add(&alp->attribs, a_new(&at_alp));
        alp_data *ad = (alp_data *)a->data.v;
        ad->mage = mage;
        ad->target = opfer;
    }

  {
      /* Wenn der Alp stirbt, den Magier nachrichtigen */
      add_trigger(&alp->attribs, "destroy", trigger_unitmessage(mage,
          "trigger_alp_destroy", MSG_EVENT, ML_INFO));
      /* Wenn Opfer oder Magier nicht mehr existieren, dann stirbt der Alp */
      add_trigger(&mage->attribs, "destroy", trigger_killunit(alp));
      add_trigger(&opfer->attribs, "destroy", trigger_killunit(alp));
  }
  msg = msg_message("summon_alp_effect", "mage alp target", mage, alp, opfer);
  r_addmessage(r, mage->faction, msg);
  msg_release(msg);

  return cast_level;
}

void alp_findet_opfer(unit * alp, region * r)
{
    curse *c;
    attrib *a = a_find(alp->attribs, &at_alp);
    alp_data *ad = (alp_data *)a->data.v;
    unit *mage = ad->mage;
    unit *opfer = ad->target;
    float effect;
    message *msg;

    assert(opfer);
    assert(mage);

    /* Magier und Opfer Bescheid geben */
    msg = msg_message("alp_success", "target", opfer);
    add_message(&mage->faction->msgs, msg);
    r_addmessage(opfer->region, opfer->faction, msg);
    msg_release(msg);

    /* Relations werden in destroy_unit(alp) automatisch gelöscht.
     * Die Aktionen, die beim Tod des Alps ausgelöst werden sollen,
     * müssen jetzt aber deaktiviert werden, sonst werden sie gleich
     * beim destroy_unit(alp) ausgelöst.
     */
    a_removeall(&alp->attribs, &at_eventhandler);

    /* Alp umwandeln in Curse */
    effect = -2;
    c =
        create_curse(mage, &opfer->attribs, ct_find("worse"), 2, 2, effect,
        opfer->number);
    /* solange es noch keine spezielle alp-Antimagie gibt, reagiert der
     * auch auf normale */
    set_number(alp, 0);

    /* wenn der Magier stirbt, wird der Curse wieder vom Opfer genommen */
    add_trigger(&mage->attribs, "destroy", trigger_removecurse(c, opfer));
}

void register_alp(void)
{
    at_register(&at_alp);
}

unit *alp_target(unit * alp)
{
    alp_data *ad;
    unit *target = NULL;

    attrib *a = a_find(alp->attribs, &at_alp);

    if (a) {
        ad = (alp_data *)a->data.v;
        target = ad->target;
    }
    return target;

}
