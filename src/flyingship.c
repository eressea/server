#include <platform.h>
#include <kernel/config.h>
#include "flyingship.h"

#include <kernel/build.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/region.h>
#include <kernel/unit.h>

#include <spells/shipcurse.h>

/* libc includes */
#include <assert.h>

int levitate_ship(ship * sh, unit * mage, double power, int duration)
{
    curse *c = shipcurse_flyingship(sh, mage, power, duration);
    if (c) {
        return c->no;
    }
    return 0;
}

/* ------------------------------------------------------------- */
/* Name:       Luftschiff
* Stufe:      6
*
* Wirkung:
* Laeßt ein Schiff eine Runde lang fliegen.  Wirkt nur auf Boote und
* Langboote.
* Kombinierbar mit "Guenstige Winde", aber nicht mit "Sturmwind".
*
* Flag:
*  (ONSHIPCAST | SHIPSPELL | TESTRESISTANCE)
*/
int sp_flying_ship(castorder * co)
{
    ship *sh;
    unit *u;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;
    message *m = NULL;
    int cno;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;
    sh = pa->param[0]->data.sh;
    if (sh->type->construction->maxsize > 50) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "error_flying_ship_too_big", "ship", sh));
        return 0;
    }

    /* Duration = 1, nur diese Runde */

    cno = levitate_ship(sh, mage, power, 1);
    if (cno == 0) {
        if (is_cursed(sh->attribs, C_SHIP_FLYING, 0)) {
            /* Auf dem Schiff befindet liegt bereits so ein Zauber. */
            cmistake(mage, co->order, 211, MSG_MAGIC);
        }
        else if (is_cursed(sh->attribs, C_SHIP_SPEEDUP, 0)) {
            /* Es ist zu gefaehrlich, ein sturmgepeitschtes Schiff fliegen zu lassen. */
            cmistake(mage, co->order, 210, MSG_MAGIC);
        }
        return 0;
    }
    sh->coast = NODIRECTION;

    /* melden, 1x pro Partei */
    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next) {
        /* das sehen natuerlich auch die Leute an Land */
        if (!fval(u->faction, FFL_SELECT)) {
            fset(u->faction, FFL_SELECT);
            if (!m)
                m = msg_message("flying_ship_result", "mage ship", mage, sh);
            add_message(&u->faction->msgs, m);
        }
    }
    if (m)
        msg_release(m);
    return cast_level;
}
