#include <platform.h>
#include "flyingship.h"

#include <spells/shipcurse.h>

#include <kernel/build.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/ship.h>

#include <kernel/gamedata.h>

#include <magic.h>

#include <storage.h>

/* libc includes */
#include <assert.h>


/* ------------------------------------------------------------- */
/* Name:       Luftschiff
* Stufe:      6
*
* Wirkung:
* Laesst ein Schiff eine Runde lang fliegen.  Wirkt nur auf Boote
* bis Kapazitaet 50.
* Kombinierbar mit "Guenstige Winde", aber nicht mit "Sturmwind".
*
* Flag:
*  (ONSHIPCAST | SHIPSPELL | TESTRESISTANCE)
*/
int sp_flying_ship(castorder * co)
{
    ship *sh;
    unit *u;
    region *r;
    unit *caster;
    int cast_level;
    double power;
    spellparameter *pa;
    message *m = NULL;
    int cno;

    assert(co);
    r = co_get_region(co);
    caster = co_get_caster(co);
    cast_level = co->level;
    power = co->force;
    pa = co->par;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag)
        return 0;
    sh = pa->param[0]->data.sh;
    if (sh->number > 1 || sh->type->construction->maxsize > 50) {
        ADDMSG(&caster->faction->msgs, msg_feedback(caster, co->order,
            "error_flying_ship_too_big", "ship", sh));
        return 0;
    }

    /* Duration = 1, nur diese Runde */

    cno = levitate_ship(sh, caster, power, 1);
    if (cno == 0) {
        if (is_cursed(sh->attribs, &ct_flyingship)) {
            /* Auf dem Schiff befindet liegt bereits so ein Zauber. */
            cmistake(caster, co->order, 211, MSG_MAGIC);
        }
        else if (is_cursed(sh->attribs, &ct_shipspeedup)) {
            /* Es ist zu gefaehrlich, ein sturmgepeitschtes Schiff fliegen zu lassen. */
            cmistake(caster, co->order, 210, MSG_MAGIC);
        }
        return 0;
    }
    sh->coast = NODIRECTION;

    /* melden, 1x pro Partei */
    for (u = r->units; u; u = u->next)
        u->faction->flags &= ~FFL_SELECT;
    for (u = r->units; u; u = u->next) {
        /* das sehen natuerlich auch die Leute an Land */
        if (!(u->faction->flags & FFL_SELECT)) {
            u->faction->flags |= FFL_SELECT;
            if (!m) {
                m = msg_message("flying_ship_result", "mage ship", caster, sh);
            }
            add_message(&u->faction->msgs, m);
        }
    }
    if (m) {
        msg_release(m);
    }
    return cast_level;
}

static int flyingship_read(gamedata * data, curse * c, void *target)
{
    ship *sh = (ship *)target;
    c->data.v = sh;
    if (data->version < FOSS_VERSION) {
        sh->flags |= SF_FLYING;
        return 0;
    }
    assert(sh->flags & SF_FLYING);
    return 0;
}

static int flyingship_write(storage * store, const curse * c,
    const void *target)
{
    const ship *sh = (const ship *)target;
    assert(sh->flags & SF_FLYING);
    return 0;
}

static int flyingship_age(curse * c)
{
    ship *sh = (ship *)c->data.v;
    if (sh && c->duration == 1) {
        sh->flags &= ~SF_FLYING;
        return 1;
    }
    return 0;
}

const struct curse_type ct_flyingship = { "flyingship",
CURSETYP_NORM, 0, NO_MERGE, cinfo_ship, NULL, flyingship_read,
flyingship_write, NULL, flyingship_age
};

void register_flyingship(void)
{
    ct_register(&ct_flyingship);
}

bool flying_ship(const ship * sh)
{
    if (sh->type->flags & SFL_FLY)
        return true;
    if (sh->flags & SF_FLYING)
        return true;
    return false;
}

static curse *shipcurse_flyingship(ship * sh, unit * mage, double power, int duration)
{
    curse *c;
    if (sh->attribs) {
        if (curse_active(get_curse(sh->attribs, &ct_flyingship))) {
            return NULL;
        }
        if (is_cursed(sh->attribs, &ct_shipspeedup)) {
            return NULL;
        }
    }
    /* mit C_SHIP_NODRIFT haben wir kein Problem */
    c = create_curse(mage, &sh->attribs, &ct_flyingship, power, duration, 0.0, 0);
    if (c) {
        c->data.v = sh;
        if (c->duration > 0) {
            sh->flags |= SF_FLYING;
        }
    }
    return c;
}

int levitate_ship(ship * sh, unit * mage, double power, int duration)
{
    curse *c = shipcurse_flyingship(sh, mage, power, duration);
    if (c) {
        return c->no;
    }
    return 0;
}

