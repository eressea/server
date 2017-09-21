/*
 *
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

#include "guard.h"
#include "spy.h"
#include "vortex.h"
#include "laws.h"
#include "spells.h"
#include "direction.h"
#include "randenc.h"
#include "monsters.h"
#include "teleport.h"

#include <attributes/attributes.h>

#include <spells/borders.h>
#include <spells/buildingcurse.h>
#include <spells/regioncurse.h>
#include <spells/unitcurse.h>
#include <spells/shipcurse.h>
#include <spells/combatspells.h>
#include <spells/flyingship.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/connection.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/objtypes.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>
#include <kernel/xmlreader.h>

#include <races/races.h>

/* util includes */
#include <util/assert.h>
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/language.h>
#include <util/message.h>
#include <util/parser.h>
#include <util/umlaut.h>
#include <util/functions.h>
#include <util/lists.h>
#include <util/rand.h>
#include <util/log.h>
#include <util/nrmessage.h>
#include <util/bsdstring.h>
#include <util/variant.h>
#include <util/goodies.h>
#include <util/resolve.h>
#include <util/rng.h>

#include <storage.h>

/* libc includes */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* triggers includes */
#include <triggers/changefaction.h>
#include <triggers/changerace.h>
#include <triggers/createcurse.h>
#include <triggers/createunit.h>
#include <triggers/killunit.h>
#include <triggers/timeout.h>

/* attributes includes */
#include <attributes/targetregion.h>
#include <attributes/hate.h>
/* ----------------------------------------------------------------------- */

#if defined(_MSC_VER) && _MSC_VER >= 1900
# pragma warning(disable: 4774) // TODO: remove this
#endif

static double zero_effect = 0.0;

/* ----------------------------------------------------------------------- */

static void report_spell(unit * mage, region * r, message * msg)
{
    r_addmessage(r, NULL, msg);
    if (mage && mage->region != r) {
        add_message(&mage->faction->msgs, msg);
    }
}

static void report_failure(unit * mage, struct order *ord)
{
    /* Fehler: "Der Zauber schlaegt fehl" */
    cmistake(mage, ord, 180, MSG_MAGIC);
}

/* ------------------------------------------------------------- */
/* Spruchanalyse - Ausgabe von curse->info und curse->name       */
/* ------------------------------------------------------------- */

static double curse_chance(const struct curse *c, double force)
{
    return 1.0 + (force - c->vigour) * 0.1;
}

static void magicanalyse_region(region * r, unit * mage, double force)
{
    attrib *a;
    bool found = false;

    for (a = r->attribs; a; a = a->next) {
        curse *c = (curse *)a->data.v;
        double probability;
        int mon;

        if (a->type != &at_curse)
            continue;

        /* ist der curse schwaecher als der Analysezauber, so ergibt sich
         * mehr als 100% probability und damit immer ein Erfolg. */
        probability = curse_chance(c, force);
        mon = c->duration + (rng_int() % 10) - 5;
        mon = MAX(1, mon);
        found = true;

        if (chance(probability)) {  /* Analyse geglueckt */
            if (c_flags(c) & CURSE_NOAGE) {
                ADDMSG(&mage->faction->msgs, msg_message("analyse_region_noage",
                    "mage region curse", mage, r, c->type));
            }
            else {
                ADDMSG(&mage->faction->msgs, msg_message("analyse_region_age",
                    "mage region curse months", mage, r, c->type, mon));
            }
        }
        else {
            ADDMSG(&mage->faction->msgs, msg_message("analyse_region_fail",
                "mage region", mage, r));
        }
    }
    if (!found) {
        ADDMSG(&mage->faction->msgs, msg_message("analyse_region_nospell",
            "mage region", mage, r));
    }
}

static void magicanalyse_unit(unit * u, unit * mage, double force)
{
    attrib *a;
    bool found = false;

    for (a = u->attribs; a; a = a->next) {
        curse *c;
        double probability;
        int mon;
        if (a->type != &at_curse)
            continue;

        c = (curse *)a->data.v;
        /* ist der curse schwaecher als der Analysezauber, so ergibt sich
         * mehr als 100% probability und damit immer ein Erfolg. */
        probability = curse_chance(c, force);
        mon = c->duration + (rng_int() % 10) - 5;
        mon = MAX(1, mon);

        if (chance(probability)) {  /* Analyse geglueckt */
            if (c_flags(c) & CURSE_NOAGE) {
                ADDMSG(&mage->faction->msgs, msg_message("analyse_unit_noage",
                    "mage unit curse", mage, u, c->type));
            }
            else {
                ADDMSG(&mage->faction->msgs, msg_message("analyse_unit_age",
                    "mage unit curse months", mage, u, c->type, mon));
            }
        }
        else {
            ADDMSG(&mage->faction->msgs, msg_message("analyse_unit_fail", "mage unit",
                mage, u));
        }
    }
    if (!found) {
        ADDMSG(&mage->faction->msgs, msg_message("analyse_unit_nospell",
            "mage target", mage, u));
    }
}

static void magicanalyse_building(building * b, unit * mage, double force)
{
    attrib *a;
    bool found = false;

    for (a = b->attribs; a; a = a->next) {
        curse *c;
        double probability;
        int mon;

        if (a->type != &at_curse)
            continue;

        c = (curse *)a->data.v;
        /* ist der curse schwaecher als der Analysezauber, so ergibt sich
         * mehr als 100% probability und damit immer ein Erfolg. */
        probability = curse_chance(c, force);
        mon = c->duration + (rng_int() % 10) - 5;
        mon = MAX(1, mon);

        if (chance(probability)) {  /* Analyse geglueckt */
            if (c_flags(c) & CURSE_NOAGE) {
                ADDMSG(&mage->faction->msgs, msg_message("analyse_building_age",
                    "mage building curse", mage, b, c->type));
            }
            else {
                ADDMSG(&mage->faction->msgs, msg_message("analyse_building_age",
                    "mage building curse months", mage, b, c->type, mon));
            }
        }
        else {
            ADDMSG(&mage->faction->msgs, msg_message("analyse_building_fail",
                "mage building", mage, b));
        }
    }
    if (!found) {
        ADDMSG(&mage->faction->msgs, msg_message("analyse_building_nospell",
            "mage building", mage, b));
    }

}

static void magicanalyse_ship(ship * sh, unit * mage, double force)
{
    attrib *a;
    bool found = false;

    for (a = sh->attribs; a; a = a->next) {
        curse *c;
        double probability;
        int mon;
        if (a->type != &at_curse)
            continue;

        c = (curse *)a->data.v;
        /* ist der curse schwaecher als der Analysezauber, so ergibt sich
         * mehr als 100% probability und damit immer ein Erfolg. */
        probability = curse_chance(c, force);
        mon = c->duration + (rng_int() % 10) - 5;
        mon = MAX(1, mon);

        if (chance(probability)) {  /* Analyse geglueckt */
            if (c_flags(c) & CURSE_NOAGE) {
                ADDMSG(&mage->faction->msgs, msg_message("analyse_ship_noage",
                    "mage ship curse", mage, sh, c->type));
            }
            else {
                ADDMSG(&mage->faction->msgs, msg_message("analyse_ship_age",
                    "mage ship curse months", mage, sh, c->type, mon));
            }
        }
        else {
            ADDMSG(&mage->faction->msgs, msg_message("analyse_ship_fail", "mage ship",
                mage, sh));

        }
    }
    if (!found) {
        ADDMSG(&mage->faction->msgs, msg_message("analyse_ship_nospell",
            "mage ship", mage, sh));
    }

}

static int break_curse(attrib ** alist, int cast_level, double force, curse * c)
{
    int succ = 0;
    /*  attrib **a = a_find(*ap, &at_curse); */
    attrib **ap = alist;

    while (*ap && force > 0) {
        curse *c1;
        attrib *a = *ap;
        if (a->type != &at_curse) {
            do {
                ap = &(*ap)->next;
            } while (*ap && a->type == (*ap)->type);
            continue;
        }
        c1 = (curse *)a->data.v;

        /* Immunitaet pruefen */
        if (c_flags(c1) & CURSE_IMMUNE) {
            do {
                ap = &(*ap)->next;
            } while (*ap && a->type == (*ap)->type);
            continue;
        }

        /* Wenn kein spezieller cursetyp angegeben ist, soll die Antimagie
         * auf alle Verzauberungen wirken. Ansonsten pruefe, ob der Curse vom
         * richtigen Typ ist. */
        if (!c || c == c1) {
            double remain = destr_curse(c1, cast_level, force);
            if (remain < force) {
                succ = cast_level;
                force = remain;
            }
            if (c1->vigour <= 0) {
                a_remove(alist, a);
            }
        }
        if (*ap == a)
            ap = &a->next;
    }
    return succ;
}

int report_action(region * r, unit * actor, message * msg, int flags)
{
    int result = 0;
    unit *u;
    int view = flags & (ACTION_CANSEE | ACTION_CANNOTSEE);

    /* melden, 1x pro Partei */
    if (flags & ACTION_RESET) {
        freset(actor->faction, FFL_SELECT);
        for (u = r->units; u; u = u->next)
            freset(u->faction, FFL_SELECT);
    }
    if (view) {
        for (u = r->units; u; u = u->next) {
            if (!fval(u->faction, FFL_SELECT)) {
                bool show = u->faction == actor->faction;
                fset(u->faction, FFL_SELECT);
                if (view == ACTION_CANSEE) {
                    /* Bei Fernzaubern sieht nur die eigene Partei den Magier */
                    show = show || (r == actor->region
                        && cansee(u->faction, r, actor, 0));
                }
                else if (view == ACTION_CANNOTSEE) {
                    show = !show && !(r == actor->region
                        && cansee(u->faction, r, actor, 0));
                }
                else {
                    /* the unliely (or lazy) case */
                    show = true;
                }

                if (show) {
                    r_addmessage(r, u->faction, msg);
                }
                else {                /* Partei des Magiers, sieht diesen immer */
                    result = 1;
                }
            }
        }
        /* Ist niemand von der Partei des Magiers in der Region, dem Magier
        * nochmal gesondert melden */
        if ((flags & ACTION_CANSEE) && !fval(actor->faction, FFL_SELECT)) {
            add_message(&actor->faction->msgs, msg);
        }
    }
    return result;
}

/* ------------------------------------------------------------- */
/* Report a spell's effect to the units in the region.
*/

static void
report_effect(region * r, unit * mage, message * seen, message * unseen)
{
    int err = report_action(r, mage, seen, ACTION_RESET | ACTION_CANSEE);
    UNUSED_ARG(unseen);
    if (err) {
        report_action(r, mage, seen, ACTION_CANNOTSEE);
    }
}

/* ------------------------------------------------------------- */
/* Die Spruchfunktionen */
/* ------------------------------------------------------------- */
/* Meldungen:
 *
 * Fehlermeldungen sollten als MSG_MAGIC, level ML_MISTAKE oder
 * ML_WARN ausgegeben werden. (stehen im Kopf der Auswertung unter
 * Zauberwirkungen)

 sprintf(buf, "%s in %s: 'ZAUBER %s': [hier die Fehlermeldung].",
 unitname(mage), regionname(mage->region, mage->faction), sa->strings[0]);
 add_message(0, mage->faction, buf,  MSG_MAGIC, ML_MISTAKE);

 * Allgemein sichtbare Auswirkungen in der Region sollten als
 * Regionsereignisse auch dort auftauchen.

 {
 message * seen = msg_message("harvest_effect", "mage", mage);
 message * unseen = msg_message("harvest_effect", "mage", NULL);
 report_effect(r, mage, seen, unseen);
 }

 * Meldungen an den Magier ueber Erfolg sollten, wenn sie nicht als
 * Regionsereigniss auftauchen, als MSG_MAGIC level ML_INFO unter
 * Zauberwirkungen gemeldet werden. Direkt dem Magier zuordnen (wie
 * Botschaft an Einheit) ist derzeit nicht moeglich.
 * ACHTUNG! r muss nicht die Region des Magier sein! (FARCASTING)
 *
 * Parameter:
 * die Struct castorder *co ist in magic.h deklariert
 * die Parameterliste spellparameter *pa = co->par steht dort auch.
 *
 */

 /* ------------------------------------------------------------- */
 /* Name:    Vertrauter
  * Stufe:   10
  *
  * Wirkung:
  * Der Magier beschwoert einen Vertrauten, ein kleines Tier, welches
  * dem Magier zu Diensten ist.  Der Magier kann durch die Augen des
  * Vertrauten sehen, und durch den Vertrauten zaubern, allerdings nur
  * mit seiner halben Stufe. Je nach Vertrautem erhaelt der Magier
  * evtl diverse Skillmodifikationen.  Der Typ des Vertrauten ist
  * zufaellig bestimmt, wird aber durch Magiegebiet und Rasse beeinflußt.
  * "Tierische" Vertraute brauchen keinen Unterhalt.
  *
  * Ein paar Moeglichkeiten:
  *       Magieg.  Rasse Besonderheiten
  * Eule   Tybied  -/-   fliegt, Auraregeneration
  * Rabe  Ilaun  -/-   fliegt
  * Imp    Draig -/-   Magieresistenz?
  * Fuchs  Gwyrrd  -/-   Wahrnehmung
  * ????   Cerddor -/-   ???? (Singvogel?, Papagei?)
  * Adler  -/-   -/-   fliegt, +Wahrnehmung, =^=Adlerauge-Spruch?
  * Kraehe  -/-   -/-   fliegt, +Tarnung (weil unauffaellig)
  * Delphin  -/-   Meerm.  schwimmt
  * Wolf   -/-   Ork
  * Hund   -/-   Mensch  kann evtl BEWACHE ausfuehren
  * Ratte  -/-   Goblin
  * Albatros -/-   -/-   fliegt, kann auf Ozean "landen"
  * Affe   -/-   -/-   kann evtl BEKLAUE ausfuehren
  * Goblin -/-   !Goblin normale Einheit
  * Katze  -/-   !Katze  normale Einheit
  * Daemon  -/-   !Daemon  normale Einheit
  *
  * Spezielle V. fuer Katzen, Trolle, Elfen, Daemonen, Insekten, Zwerge?
  */

static const race *select_familiar(const race * magerace, magic_t magiegebiet)
{
    const race *retval = 0;
    int rnd = rng_int() % 100;

    assert(magerace->familiars[0]);
    if (rnd >= 70) {
        retval = magerace->familiars[magiegebiet];
        assert(retval);
    }
    else {
        retval = magerace->familiars[0];
    }

    if (!retval || rnd < 3) {
        race_list *familiarraces = get_familiarraces();
        unsigned int maxlen = listlen(familiarraces);
        if (maxlen > 0) {
            race_list *rclist = familiarraces;
            unsigned int index = rng_uint() % maxlen;
            while (index-- > 0) {
                rclist = rclist->next;
            }
            retval = rclist->data;
        }
    }

    if (!retval) {
        retval = magerace->familiars[0];
    }
    if (!retval) {
        log_error("select_familiar: No familiar (not even a default) defined for %s.\n", magerace->_name);
    }
    return retval;
}

/* ------------------------------------------------------------- */
/* der Vertraue des Magiers */

static void make_familiar(unit * familiar, unit * mage)
{
    /* skills and spells: */
    const struct equipment *eq;
    char eqname[64];
    const race * rc = u_race(familiar);
    snprintf(eqname, sizeof(eqname), "fam_%s", rc->_name);
    eq = get_equipment(eqname);
    if (eq != NULL) {
        equip_items(&familiar->items, eq);
    }
    else {
        log_info("could not perform initialization for familiar %s.\n", rc->_name);
    }

    /* triggers: */
    create_newfamiliar(mage, familiar);

    /* Hitpoints nach Talenten korrigieren, sonst starten vertraute
     * mit Ausdauerbonus verwundet */
    familiar->hp = unit_max_hp(familiar);
}

static int sp_summon_familiar(castorder * co)
{
    unit *familiar;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    const race *rc;
    int sk;
    int dh, dh1;
    size_t bytes;
    message *msg;
    char zText[2048], *bufp = zText;
    size_t size = sizeof(zText) - 1;

    if (get_familiar(mage) != NULL) {
        cmistake(mage, co->order, 199, MSG_MAGIC);
        return 0;
    }
    rc = select_familiar(mage->_race, mage->faction->magiegebiet);
    if (rc == NULL) {
        log_error("could not find suitable familiar for %s.\n", mage->faction->race->_name);
        return 0;
    }

    if (fval(rc, RCF_SWIM) && !fval(rc, RCF_WALK)) {
        int coasts = is_coastregion(r);
        int dir;
        if (coasts == 0) {
            cmistake(mage, co->order, 229, MSG_MAGIC);
            return 0;
        }

        /* In welcher benachbarten Ozeanregion soll der Familiar erscheinen? */
        coasts = rng_int() % coasts;
        dh = -1;
        for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
            region *rn = rconnect(r, dir);
            if (rn && fval(rn->terrain, SEA_REGION)) {
                dh++;
                if (dh == coasts) {
                    r = rconnect(r, dir);
                    break;
                }
            }
        }
    }

    msg = msg_message("familiar_name", "unit", mage);
    nr_render(msg, mage->faction->locale, zText, sizeof(zText), mage->faction);
    msg_release(msg);
    familiar = create_unit(r, mage->faction, 1, rc, 0, zText, mage);
    setstatus(familiar, ST_FLEE);
    fset(familiar, UFL_LOCKED);
    make_familiar(familiar, mage);

    dh = 0;
    dh1 = 0;
    for (sk = 0; sk < MAXSKILLS; ++sk) {
        if (skill_enabled(sk) && rc->bonus[sk] > -5)
            dh++;
    }

    for (sk = 0; sk < MAXSKILLS; sk++) {
        if (skill_enabled(sk) && rc->bonus[sk] > -5) {
            dh--;
            if (dh1 == 0) {
                dh1 = 1;
            }
            else {
                if (dh == 0) {
                    bytes =
                        strlcpy(bufp, (const char *)LOC(mage->faction->locale,
                            "list_and"), size);
                }
                else {
                    bytes = strlcpy(bufp, (const char *)", ", size);
                }
                assert(bytes <= INT_MAX);
                if (wrptr(&bufp, &size, (int)bytes) != 0)
                    WARN_STATIC_BUFFER();
            }
            bytes =
                strlcpy(bufp, (const char *)skillname((skill_t)sk, mage->faction->locale),
                    size);
            assert(bytes <= INT_MAX);
            if (wrptr(&bufp, &size, (int)bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }
    ADDMSG(&mage->faction->msgs, msg_message("familiar_describe",
        "mage race skills", mage, rc, zText));
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Zerstoere Magie
 * Wirkung:
 *  Zerstoert alle Zauberwirkungen auf dem Objekt. Jeder gebrochene
 *  Zauber verbraucht c->vigour an Zauberkraft. Wird der Spruch auf
 *  einer geringeren Stufe gezaubert, als der Zielzauber an c->vigour
 *  hat, so schlaegt die Aufloesung mit einer von der Differenz abhaengigen
 *  Chance fehl.  Auch dann wird force verbraucht, der Zauber jedoch nur
 *  abgeschwaecht.
 *
 * Flag:
 *  (FARCASTING|SPELLLEVEL|ONSHIPCAST|TESTCANSEE)
 * */
static int sp_destroy_magic(castorder * co)
{
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    spellparameter *pa = co->par;
    curse *c = NULL;
    char ts[80];
    attrib **ap;
    int obj;
    int succ;

    /* da jeder Zauber force verbraucht und der Zauber auf alles und nicht
     * nur einen Spruch wirken soll, wird die Wirkung hier verstaerkt */
    force *= 4;

    /* Objekt ermitteln */
    obj = pa->param[0]->typ;

    switch (obj) {
    case SPP_REGION:
    {
        /* region *tr = pa->param[0]->data.r; -- farcasting! */
        region *tr = co_get_region(co);
        ap = &tr->attribs;
        write_regionname(tr, mage->faction, ts, sizeof(ts));
        break;
    }
    case SPP_TEMP:
    case SPP_UNIT:
    {
        unit *u;
        u = pa->param[0]->data.u;
        ap = &u->attribs;
        write_unitname(u, ts, sizeof(ts));
        break;
    }
    case SPP_BUILDING:
    {
        building *b;
        b = pa->param[0]->data.b;
        ap = &b->attribs;
        write_buildingname(b, ts, sizeof(ts));
        break;
    }
    case SPP_SHIP:
    {
        ship *sh;
        sh = pa->param[0]->data.sh;
        ap = &sh->attribs;
        write_shipname(sh, ts, sizeof(ts));
        break;
    }
    default:
        return 0;
    }

    succ = break_curse(ap, cast_level, force, c);

    if (succ) {
        ADDMSG(&mage->faction->msgs, msg_message("destroy_magic_effect",
            "unit region command succ target", mage, mage->region, co->order, succ,
            ts));
    }
    else {
        ADDMSG(&mage->faction->msgs, msg_message("destroy_magic_noeffect",
            "unit region command", mage, mage->region, co->order));
    }

    return MAX(succ, 1);
}

/* ------------------------------------------------------------- */
/* Name:    Transferiere Aura
 * Stufe:  variabel
 * Gebiet:  alle
 * Kategorie:     Einheit, positiv
 * Wirkung:
 *   Mit Hilfe dieses Zauber kann der Magier eigene Aura im Verhaeltnis
 *   2:1 auf einen anderen Magier des gleichen Magiegebietes oder (nur
 *   bei Tybied) im Verhaeltnis 3:1 auf einen Magier eines anderen
 *   Magiegebietes uebertragen.
 *
 * Syntax:
 *  "ZAUBERE <spruchname> <Einheit-Nr> <investierte Aura>"
 *  "ui"
 * Flags:
 *  (UNITSPELL|ONSHIPCAST)
 * */

static int sp_transferaura(castorder * co)
{
    int aura, gain, multi = 2;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;
    unit *u;
    sc_mage *scm_dst, *scm_src = get_mage(mage);

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
     * abbrechen aber kosten lassen */
    if (pa->param[0]->flag == TARGET_RESISTS)
        return cast_level;

    /* Wieviel Transferieren? */
    aura = pa->param[1]->data.i;
    u = pa->param[0]->data.u;
    scm_dst = get_mage(u);

    if (scm_dst == NULL) {
        /* "Zu dieser Einheit kann ich keine Aura uebertragen." */
        cmistake(mage, co->order, 207, MSG_MAGIC);
        return 0;
    }
    else if (scm_src->magietyp == M_TYBIED) {
        if (scm_src->magietyp != scm_dst->magietyp)
            multi = 3;
    }
    else if (scm_src->magietyp == M_GRAY) {
        if (scm_src->magietyp != scm_dst->magietyp)
            multi = 4;
    }
    else if (scm_dst->magietyp != scm_src->magietyp) {
        /* "Zu dieser Einheit kann ich keine Aura uebertragen." */
        cmistake(mage, co->order, 207, MSG_MAGIC);
        return 0;
    }

    if (aura < multi) {
        /* "Auraangabe fehlerhaft." */
        cmistake(mage, co->order, 208, MSG_MAGIC);
        return 0;
    }

    gain = MIN(aura, scm_src->spellpoints) / multi;
    scm_src->spellpoints -= gain * multi;
    scm_dst->spellpoints += gain;

    /*  sprintf(buf, "%s transferiert %d Aura auf %s", unitname(mage),
          gain, unitname(u)); */
    ADDMSG(&mage->faction->msgs, msg_message("auratransfer_success",
        "unit target aura", mage, u, gain));
    return cast_level;
}

/* ------------------------------------------------------------- */
/* DRUIDE */
/* ------------------------------------------------------------- */
/* Name:       Guenstige Winde
 * Stufe:      4
 * Gebiet:     Gwyrrd
 * Wirkung:
 * Schiffsbewegung +1, kein Abtreiben.  Haelt (Stufe) Runden an.
 * Kombinierbar mit "Sturmwind" (das +1 wird dadurch aber nicht
 * verdoppelt), und "Luftschiff".
 *
 * Flags:
 * (SHIPSPELL|ONSHIPCAST|SPELLLEVEL|TESTRESISTANCE)
 */

static int sp_goodwinds(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    int duration = cast_level + 1;
    spellparameter *pa = co->par;
    message *m;
    ship *sh;
    unit *u;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    sh = pa->param[0]->data.sh;

    /* keine Probleme mit C_SHIP_SPEEDUP und C_SHIP_FLYING */
    /* NODRIFT bewirkt auch +1 Geschwindigkeit */
    create_curse(mage, &sh->attribs, &ct_nodrift, power, duration,
        zero_effect, 0);

    /* melden, 1x pro Partei */
    freset(mage->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    m = msg_message("wind_effect", "mage ship", mage, sh);
    for (u = r->units; u; u = u->next) {
        if (u->ship != sh)          /* nur den Schiffsbesatzungen! */
            continue;
        if (!fval(u->faction, FFL_SELECT)) {
            r_addmessage(r, u->faction, m);
            fset(u->faction, FFL_SELECT);
        }
    }
    if (!fval(mage->faction, FFL_SELECT)) {
        r_addmessage(r, mage->faction, m);
    }
    msg_release(m);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Magischer Pfad
 * Stufe:      4
 * Gebiet:     Gwyrrd
 * Wirkung:
 *   fuer Stufe Runden wird eine (magische) Strasse erzeugt, die wie eine
 *   normale Strasse wirkt.
 *   Im Ozean schlaegt der Spruch fehl
 *
 * Flags:
 * (FARCASTING|SPELLLEVEL|REGIONSPELL|ONSHIPCAST|TESTRESISTANCE)
 */
static int sp_magicstreet(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;

    if (!fval(r->terrain, LAND_REGION)) {
        cmistake(mage, co->order, 186, MSG_MAGIC);
        return 0;
    }

    /* wirkt schon in der Zauberrunde! */
    create_curse(mage, &r->attribs, &ct_magicstreet, co->force,
        co->level + 1, zero_effect, 0);

    /* melden, 1x pro Partei */
    {
        message *seen = msg_message("path_effect", "mage region", mage, r);
        message *unseen = msg_message("path_effect", "mage region", NULL, r);
        report_effect(r, mage, seen, unseen);
        msg_release(seen);
        msg_release(unseen);
    }

    return co->level;
}

/* ------------------------------------------------------------- */
/* Name:       Erwecke Ents
 * Stufe:      10
 * Kategorie:  Beschwoerung, positiv
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Verwandelt (Stufe) Baeume in eine Gruppe von Ents, die sich fuer Stufe
 *  Runden der Partei des Druiden anschliessen und danach wieder zu
 *  Baeumen werden
 * Patzer:
 *  Monster-Ents entstehen
 *
 * Flags:
 * (SPELLLEVEL)
 */
static int sp_summonent(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    unit *u;
    attrib *a;
    int ents;

    if (rtrees(r, 2) == 0) {
        cmistake(mage, co->order, 204, MSG_EVENT);
        /* nicht ohne baeume */
        return 0;
    }

    ents = MIN((int)(power * power), rtrees(r, 2));

    u = create_unit(r, mage->faction, ents, get_race(RC_TREEMAN), 0, NULL, mage);

    a = a_new(&at_unitdissolve);
    a->data.ca[0] = 2;            /* An r->trees. */
    a->data.ca[1] = 5;            /* 5% */
    a_add(&u->attribs, a);
    fset(u, UFL_LOCKED);

    rsettrees(r, 2, rtrees(r, 2) - ents);

    /* melden, 1x pro Partei */
    {
        message *seen = msg_message("ent_effect", "mage amount", mage, ents);
        message *unseen = msg_message("ent_effect", "mage amount", NULL, ents);
        report_effect(r, mage, seen, unseen);
        msg_release(unseen);
        msg_release(seen);
    }
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Segne Steinkreis
 * Stufe:      11
 * Kategorie:  Artefakt
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Es werden zwei neue Gebaeude eingefuehrt: Steinkreis und Steinkreis
 *  (gesegnet). Ersteres kann man bauen, letzteres wird aus einem
 *  fertigen Steinkreis mittels des Zaubers erschaffen.
 *
 * Flags:
 * (BUILDINGSPELL)
 *
 */
static int sp_blessstonecircle(castorder * co)
{
    building *b;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *p = co->par;
    message *msg;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (p->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    b = p->param[0]->data.b;

    if (!is_building_type(b->type, "stonecircle")) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "error_notstonecircle", "building", b));
        return 0;
    }

    if (!building_finished(b)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "error_notcomplete", "building", b));
        return 0;
    }

    b->type = bt_find("blessedstonecircle");

    msg = msg_message("blessedstonecircle_effect", "mage building", mage, b);
    add_message(&r->msgs, msg);
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Mahlstrom
 * Stufe:      15
 * Kategorie:  Region, negativ
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Erzeugt auf See einen Mahlstrom fuer Stufe-Wochen. Jedes Schiff, das
 *  durch den Mahlstrom segelt, nimmt 0-150% Schaden. (D.h.  es hat auch
 *  eine 1/3-Chance, ohne Federlesens zu sinken.  Der Mahlstrom sollte
 *  aus den Nachbarregionen sichtbar sein.
 *
 * Flags:
 * (OCEANCASTABLE | ONSHIPCAST | REGIONSPELL | TESTRESISTANCE)
 */
static int sp_maelstrom(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    curse *c;
    int duration = (int)co->force + 1;

    if (!fval(r->terrain, SEA_REGION)) {
        cmistake(mage, co->order, 205, MSG_MAGIC);
        /* nur auf ozean */
        return 0;
    }

    /* Attribut auf Region.
     * Existiert schon ein curse, so wird dieser verstaerkt
     * (Max(Dauer), Max(Staerke))*/
    c = create_curse(mage, &r->attribs, &ct_maelstrom, co->force, duration, co->force, 0);

    /* melden, 1x pro Partei */
    if (c) {
        message *seen = msg_message("maelstrom_effect", "mage", mage);
        message *unseen = msg_message("maelstrom_effect", "mage", NULL);
        report_effect(r, mage, seen, unseen);
        msg_release(seen);
        msg_release(unseen);
    }

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Wurzeln der Magie
 * Stufe:      16
 * Kategorie:  Region, neutral
 * Gebiet:     Gwyrrd
 * Wirkung:
 *   Wandelt einen Wald permanent in eine Mallornregion
 *
 * Flags:
 * (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
static int sp_mallorn(castorder * co)
{
    region *r = co_get_region(co);
    int cast_level = co->level;
    unit *mage = co->magician.u;

    if (!fval(r->terrain, LAND_REGION)) {
        cmistake(mage, co->order, 186, MSG_MAGIC);
        return 0;
    }
    if (fval(r, RF_MALLORN)) {
        cmistake(mage, co->order, 191, MSG_MAGIC);
        return 0;
    }

    /* half the trees will die */
    rsettrees(r, 2, rtrees(r, 2) / 2);
    rsettrees(r, 1, rtrees(r, 1) / 2);
    rsettrees(r, 0, rtrees(r, 0) / 2);
    fset(r, RF_MALLORN);

    /* melden, 1x pro Partei */
    {
        message *seen = msg_message("mallorn_effect", "mage", mage);
        message *unseen = msg_message("mallorn_effect", "mage", NULL);
        report_effect(r, mage, seen, unseen);
        msg_release(seen);
        msg_release(unseen);
    }

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Segen der Erde / Regentanz
 * Stufe:      1
 * Kategorie:  Region, positiv
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 *  Alle Bauern verdienen Stufe-Wochen 1 Silber mehr.
 *
 * Flags:
 * (FARCASTING | SPELLLEVEL | ONSHIPCAST | REGIONSPELL)
 */
static int sp_blessedharvest(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    int duration = (int)co->force + 1;
    /* Attribut auf Region.
     * Existiert schon ein curse, so wird dieser verstaerkt
     * (Max(Dauer), Max(Staerke))*/

    if (create_curse(mage, &r->attribs, &ct_blessedharvest, co->force,
        duration, 1.0, 0)) {
        const char * effect = co->sp->sname[0]=='b' ? "harvest_effect" : "raindance_effect";
        message *seen = msg_message(effect, "mage", mage);
        message *unseen = msg_message(effect, "mage", NULL);
        report_effect(r, mage, seen, unseen);
        msg_release(seen);
        msg_release(unseen);
    }

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Hainzauber
 * Stufe:      2
 * Kategorie:  Region, positiv
 * Gebiet:     Gwyrrd
 * Syntax:     ZAUBER [REGION x y] [STUFE 2] "Hain"
 * Wirkung:
 *     Erschafft Stufe-10*Stufe Jungbaeume
 *
 * Flag:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */

static int sp_hain(castorder * co)
{
    int trees;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;

    if (!r->land) {
        cmistake(mage, co->order, 296, MSG_MAGIC);
        return 0;
    }
    if (fval(r, RF_MALLORN)) {
        cmistake(mage, co->order, 92, MSG_MAGIC);
        return 0;
    }

    trees = lovar((int)(force * 10 * RESOURCE_QUANTITY)) + (int)force;
    rsettrees(r, 1, rtrees(r, 1) + trees);

    /* melden, 1x pro Partei */
    {
        message *seen = msg_message("growtree_effect", "mage amount", mage, trees);
        message *unseen =
            msg_message("growtree_effect", "mage amount", NULL, trees);
        report_effect(r, mage, seen, unseen);
        msg_release(seen);
        msg_release(unseen);
    }

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Segne Mallornstecken - Mallorn Hainzauber
 * Stufe:      4
 * Kategorie:  Region, positiv
 * Gebiet:     Gwyrrd
 * Syntax:     ZAUBER [REGION x y] [STUFE 4] "Segne Mallornstecken"
 * Wirkung:
 *     Erschafft Stufe-10*Stufe Jungbaeume
 *
 * Flag:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */

static int sp_mallornhain(castorder * co)
{
    int trees;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;

    if (!r->land) {
        cmistake(mage, co->order, 296, MSG_MAGIC);
        return 0;
    }
    if (!fval(r, RF_MALLORN)) {
        cmistake(mage, co->order, 91, MSG_MAGIC);
        return 0;
    }

    trees = lovar((int)(force * 10 * RESOURCE_QUANTITY)) + (int)force;
    rsettrees(r, 1, rtrees(r, 1) + trees);

    /* melden, 1x pro Partei */
    {
        message *seen = msg_message("growtree_effect", "mage amount", mage, trees);
        message *unseen =
            msg_message("growtree_effect", "mage amount", NULL, trees);
        report_effect(r, mage, seen, unseen);
        msg_release(seen);
        msg_release(unseen);
    }

    return cast_level;
}

static void fumble_ents(const castorder * co)
{
    int ents;
    unit *u;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    /* int cast_level = co->level; */
    double force = co->force;

    if (!r->land) {
        cmistake(mage, co->order, 296, MSG_MAGIC);
        return;
    }

    ents = (int)(force * 10);
    u = create_unit(r, get_monsters(), ents, get_race(RC_TREEMAN), 0, NULL, NULL);

    if (u) {
        message *unseen;

        /* 'Erfolg' melden */
        ADDMSG(&mage->faction->msgs, msg_message("regionmagic_patzer",
            "unit region command", mage, mage->region, co->order));

        /* melden, 1x pro Partei */
        unseen = msg_message("entrise", "region", r);
        report_effect(r, mage, unseen, unseen);
        msg_release(unseen);
    }
}

/* ------------------------------------------------------------- */
/* Name:       Rosthauch
 * Stufe:      3
 * Kategorie:  Einheit, negativ
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Zerstoert zwischen Stufe und Stufe*10 Eisenwaffen
 *
 * Flag:
 * (FARCASTING | SPELLLEVEL | UNITSPELL | TESTCANSEE | TESTRESISTANCE)
 */
 /* Syntax: ZAUBER [REGION x y] [STUFE 2] "Rosthauch" 1111 2222 3333 */

typedef struct iron_weapon {
    const struct item_type *type;
    const struct item_type *rusty;
    float chance;
    struct iron_weapon *next;
} iron_weapon;

static iron_weapon *ironweapons = NULL;

void
add_ironweapon(const struct item_type *type, const struct item_type *rusty,
    float chance)
{
    iron_weapon *iweapon = malloc(sizeof(iron_weapon));
    assert_alloc(iweapon);
    iweapon->type = type;
    iweapon->rusty = rusty;
    iweapon->chance = chance;
    iweapon->next = ironweapons;
    ironweapons = iweapon;
}

static int sp_rosthauch(castorder * co)
{
    int n;
    int success = 0;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    int force = (int)co->force;
    spellparameter *pa = co->par;

    if (ironweapons == NULL) {
        add_ironweapon(it_find("sword"), it_find("rustysword"), 1.0);
        add_ironweapon(it_find("axe"), it_find("rustyaxe"), 1.0);
        add_ironweapon(it_find("greatsword"), it_find("rustygreatsword"), 1.0);
        add_ironweapon(it_find("halberd"), it_find("rustyhalberd"), 0.5f);
#ifndef NO_RUSTY_ARMOR
        add_ironweapon(it_find("shield"), it_find("rustyshield"), 0.5f);
        add_ironweapon(it_find("chainmail"), it_find("rustychainmail"), 0.2f);
#endif
    }

    if (force > 0) {
        force = rng_int() % ((int)(force * 10)) + force;
    }
    /* fuer jede Einheit */
    for (n = 0; n < pa->length; n++) {
        unit *u = pa->param[n]->data.u;
        int ironweapon = 0;
        iron_weapon *iweapon = ironweapons;

        if (force <= 0)
            break;
        if (pa->param[n]->flag & (TARGET_RESISTS | TARGET_NOTFOUND))
            continue;

        for (; iweapon != NULL; iweapon = iweapon->next) {
            item **ip = i_find(&u->items, iweapon->type);
            if (*ip) {
                float chance = (float)MIN((*ip)->number, force);
                if (iweapon->chance < 1.0) {
                    chance *= iweapon->chance;
                }
                if (chance > 0) {
                    int ichange = (int)chance;
                    force -= ichange;
                    ironweapon += ichange;
                    i_change(ip, iweapon->type, -ichange);
                    if (iweapon->rusty) {
                        i_change(&u->items, iweapon->rusty, ichange);
                    }
                }
            }
            if (force <= 0)
                break;
        }

        if (ironweapon > 0) {
            /* {$mage mage} legt einen Rosthauch auf {target}. {amount} Waffen
             * wurden vom Rost zerfressen */
            ADDMSG(&mage->faction->msgs, msg_message("rust_effect",
                "mage target amount", mage, u, ironweapon));
            ADDMSG(&u->faction->msgs, msg_message("rust_effect",
                "mage target amount",
                cansee(u->faction, r, mage, 0) ? mage : NULL, u, ironweapon));
            success += ironweapon;
        }
        else {
            /* {$mage mage} legt einen Rosthauch auf {target}, doch der
             * Rosthauch fand keine Nahrung */
            ADDMSG(&mage->faction->msgs, msg_message("rust_fail", "mage target", mage,
                u));
        }
    }
    /* in success stehen nun die insgesamt zerstoerten Waffen. Im
     * unguenstigsten Fall kann pro Stufe nur eine Waffe verzaubert werden,
     * darum wird hier nur fuer alle Faelle in denen noch weniger Waffen
     * betroffen wurden ein Kostennachlass gegeben */
    return MIN(success, cast_level);
}

/* ------------------------------------------------------------- */
/* Name:       Kaelteschutz
 * Stufe:      3
 * Kategorie:  Einheit, positiv
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 *  schuetzt ein bis mehrere Einheiten mit bis zu Stufe*10 Insekten vor
 *  den Auswirkungen der Kaelte. Sie koennen Gletscher betreten und dort
 *  ganz normal alles machen. Die Wirkung haelt Stufe Wochen an
 *  Insekten haben in Gletschern den selben Malus wie in Bergen. Zu
 *  lange drin, nicht mehr aendern
 *
 * Flag:
 * (UNITSPELL | SPELLLEVEL | ONSHIPCAST | TESTCANSEE)
 */
 /* Syntax: ZAUBER [STUFE n] "Kaelteschutz" eh1 [eh2 [eh3 [...]]] */

static int sp_kaelteschutz(castorder * co)
{
    unit *u;
    int n, i = 0;
    int men;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = MAX(cast_level, (int)force) + 1;
    spellparameter *pa = co->par;
    double effect;

    force *= 10;                  /* 10 Personen pro Force-Punkt */

    /* fuer jede Einheit in der Kommandozeile */
    for (n = 0; n < pa->length; n++) {
        if (force < 1)
            break;

        if (pa->param[n]->flag == TARGET_RESISTS
            || pa->param[n]->flag == TARGET_NOTFOUND)
            continue;

        u = pa->param[n]->data.u;

        if (force < u->number) {
            men = (int)force;
        }
        else {
            men = u->number;
        }

        effect = 1;
        create_curse(mage, &u->attribs, &ct_insectfur, (float)cast_level,
            duration, effect, men);

        force -= u->number;
        ADDMSG(&mage->faction->msgs, msg_message("heat_effect", "mage target", mage,
            u));
        if (u->faction != mage->faction)
            ADDMSG(&u->faction->msgs, msg_message("heat_effect", "mage target",
                cansee(u->faction, r, mage, 0) ? mage : NULL, u));
        i = cast_level;
    }
    /* Erstattung? */
    return i;
}

/* ------------------------------------------------------------- */
/* Name:       Verwuenschung, Funkenregen, Naturfreund, ...
 * Stufe:      1
 * Kategorie:  Einheit, rein visuell
 * Gebiet:     Alle
 *
 * Wirkung:
 * Die Einheit wird von einem magischen Effekt heimgesucht, der in ihrer
 * Beschreibung auftaucht, aber nur visuellen Effekt hat.
 *
 * Flag:
 * (UNITSPELL | TESTCANSEE | SPELLLEVEL)
 */
 /* Syntax: ZAUBER "Funkenregen" eh1 */

static int sp_sparkle(castorder * co)
{
    unit *u;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;
    int duration = cast_level + 1;
    double effect;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
     * abbrechen aber kosten lassen */
    if (pa->param[0]->flag == TARGET_RESISTS)
        return cast_level;

    u = pa->param[0]->data.u;
    effect = (float)(rng_int() % 0xffffff);
    create_curse(mage, &u->attribs, &ct_sparkle, (float)cast_level,
        duration, effect, u->number);

    ADDMSG(&mage->faction->msgs, msg_message("sparkle_effect", "mage target",
        mage, u));
    if (u->faction != mage->faction) {
        ADDMSG(&u->faction->msgs, msg_message("sparkle_effect", "mage target", mage,
            u));
    }

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Eisengolem
 * Stufe:      2
 * Kategorie:  Beschwoerung, positiv
 * Gebiet:     Gwyrrd
 * Wirkung:
 *   Erschafft eine Einheit Eisengolems mit Stufe*8 Golems.  Jeder Golem
 *   hat jede Runde eine Chance von 15% zu Staub zu zerfallen.  Gibt man
 *   den Golems den Befehl 'mache Schwert/Bihaender' oder 'mache
 *   Schild/Kettenhemd/Plattenpanzer', so werden pro Golem 5 Eisenbarren
 *   verbaut und der Golem loest sich auf.
 *
 *   Golems sind zu langsam um wirklich im Kampf von Nutzen zu sein.
 *   Jedoch fangen sie eine Menge Schaden auf und sollten sie zufaellig
 *   treffen, so ist der Schaden fast immer toedlich.  (Eisengolem: HP
 *   50, AT 4, PA 2, Ruestung 2(KH), 2d10+4 TP, Magieresistenz 0.25)
 *
 *   Golems nehmen nix an und geben nix.  Sie bewegen sich immer nur 1
 *   Region weit und ziehen aus Strassen keinen Nutzen.  Ein Golem wiegt
 *   soviel wie ein Stein. Kann nicht im Sumpf gezaubert werden
 *
 * Flag:
 *  (SPELLLEVEL)
 *
 * #define GOLEM_IRON   4
 */

static int sp_create_irongolem(castorder * co)
{
    unit *u2;
    attrib *a;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int number = lovar(force * 8 * RESOURCE_QUANTITY);
    static int cache;
    static const race * golem_rc;

    if (rc_changed(&cache)) {
        golem_rc = rc_find("irongolem");
    }

    if (number < 1) {
        number = 1;
    }

    if (r->terrain == newterrain(T_SWAMP)) {
        cmistake(mage, co->order, 188, MSG_MAGIC);
        return 0;
    }

    u2 = create_unit(r, mage->faction, number, golem_rc, 0, NULL, mage);

    set_level(u2, SK_ARMORER, 1);
    set_level(u2, SK_WEAPONSMITH, 1);

    a = a_new(&at_unitdissolve);
    a->data.ca[0] = 0;
    a->data.ca[1] = IRONGOLEM_CRUMBLE;
    a_add(&u2->attribs, a);

    ADDMSG(&mage->faction->msgs,
        msg_message("magiccreate_effect", "region command unit amount object",
            mage->region, co->order, mage, number,
            LOC(mage->faction->locale, rc_name_s(golem_rc, (u2->number == 1) ? NAME_SINGULAR : NAME_PLURAL))));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Steingolem
 * Stufe:      1
 * Kategorie:  Beschwoerung, positiv
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Erschafft eine Einheit Steingolems mit Stufe*5 Golems. Jeder Golem
 *  hat jede Runde eine Chance von 10% zu Staub zu zerfallen.  Gibt man
 *  den Golems den Befehl 'mache Burg' oder 'mache Strasse', so werden
 *  pro Golem 10 Steine verbaut und der Golem loest sich auf.
 *
 *  Golems sind zu langsam um wirklich im Kampf von Nutzen zu sein.
 *  Jedoch fangen sie eine Menge Schaden auf und sollten sie zufaellig
 *  treffen, so ist der Schaden fast immer toedlich.  (Steingolem: HP 60,
 *  AT 4, PA 2, Ruestung 4(PP), 2d12+6 TP)
 *
 *  Golems nehmen nix an und geben nix. Sie bewegen sich immer nur 1
 *  Region weit und ziehen aus Strassen keinen Nutzen. Ein Golem wiegt
 *  soviel wie ein Stein.
 *
 *  Kann nicht im Sumpf gezaubert werden
 *
 * Flag:
 *  (SPELLLEVEL)
 *
 * #define GOLEM_STONE 4
 */
static int sp_create_stonegolem(castorder * co)
{
    unit *u2;
    attrib *a;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    int number = lovar(co->force * 5 * RESOURCE_QUANTITY);
    static int cache;
    static const race * golem_rc;

    if (rc_changed(&cache)) {
        golem_rc = rc_find("stonegolem");
    }
    if (number < 1)
        number = 1;

    if (r->terrain == newterrain(T_SWAMP)) {
        cmistake(mage, co->order, 188, MSG_MAGIC);
        return 0;
    }

    u2 =
        create_unit(r, mage->faction, number, golem_rc, 0, NULL, mage);
    set_level(u2, SK_ROAD_BUILDING, 1);
    set_level(u2, SK_BUILDING, 1);

    a = a_new(&at_unitdissolve);
    a->data.ca[0] = 0;
    a->data.ca[1] = STONEGOLEM_CRUMBLE;
    a_add(&u2->attribs, a);

    ADDMSG(&mage->faction->msgs,
        msg_message("magiccreate_effect", "region command unit amount object",
            mage->region, co->order, mage, number,
            LOC(mage->faction->locale, rc_name_s(golem_rc, (u2->number == 1) ? NAME_SINGULAR : NAME_PLURAL))));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Gro�e Duerre
 * Stufe:      17
 * Kategorie:  Region, negativ
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 *   50% alle Bauern, Pferde, Baeume sterben.
 *   Zu 25% terraform: Gletscher wird mit 50% zu Sumpf, sonst Ozean,
 *   Sumpf wird zu Steppe, Ebene zur Steppe, Steppe zur Wueste.
 * Besonderheiten:
 *  neuer Terraintyp Steppe:
 *  5000 Felder, 500 Baeume, Strasse: 250 Steine. Anlegen wie in Ebene
 *  moeglich
 *
 * Flags:
 * (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */

static void destroy_all_roads(region * r)
{
    int i;

    for (i = 0; i < MAXDIRECTIONS; i++) {
        rsetroad(r, (direction_t)i, 0);
    }
}

static int sp_great_drought(castorder * co)
{
    unit *u;
    bool terraform = false;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = 2;
    double effect;

    if (fval(r->terrain, SEA_REGION)) {
        cmistake(mage, co->order, 189, MSG_MAGIC);
        /* TODO: vielleicht einen netten Patzer hier? */
        return 0;
    }

    /* sterben */
    rsetpeasants(r, rpeasants(r) / 2);    /* evtl wuerfeln */
    rsettrees(r, 2, rtrees(r, 2) / 2);
    rsettrees(r, 1, rtrees(r, 1) / 2);
    rsettrees(r, 0, rtrees(r, 0) / 2);
    rsethorses(r, rhorses(r) / 2);

    /* Arbeitslohn = 1/4 */
    effect = 4.0;                 /* curses: higher is stronger */
    create_curse(mage, &r->attribs, &ct_drought, force, duration, effect,
        0);

    /* terraforming */
    if (rng_int() % 100 < 25) {
        terraform = true;

        switch (rterrain(r)) {
        case T_PLAIN:
            /* rsetterrain(r, T_GRASSLAND); */
            destroy_all_roads(r);
            break;

        case T_SWAMP:
            /* rsetterrain(r, T_GRASSLAND); */
            destroy_all_roads(r);
            break;
            /*
                  case T_GRASSLAND:
                  rsetterrain(r, T_DESERT);
                  destroy_all_roads(r);
                  break;
                  */
        case T_GLACIER:
            if (rng_int() % 100 < 50) {
                rsetterrain(r, T_SWAMP);
                destroy_all_roads(r);
            }
            else {                /* Ozean */
                destroy_all_roads(r);
                rsetterrain(r, T_OCEAN);
                /* Einheiten duerfen hier auf keinen Fall geloescht werden! */
                for (u = r->units; u; u = u->next) {
                    if (!u->ship) {
                        set_number(u, 0);
                    }
                }
                while (r->buildings) {
                    remove_building(&r->buildings, r->buildings);
                }
            }
            break;

        default:
            terraform = false;
            break;
        }
    }

    if (!fval(r->terrain, SEA_REGION)) {
        /* not destroying the region, so it should be safe to make this a local
         * message */
        message *msg;
        const char *mtype;
        if (r->terrain == newterrain(T_SWAMP) && terraform) {
            mtype = "drought_effect_1";
        }
        else if (!terraform) {
            mtype = "drought_effect_2";
        }
        else {
            mtype = "drought_effect_3";
        }
        msg = msg_message(mtype, "mage region", mage, r);
        add_message(&r->msgs, msg);
        msg_release(msg);
    }
    else {
        /* possible that all units here get killed so better to inform with a global
         * message */
        message *msg = msg_message("drought_effect_4", "mage region", mage, r);
        for (u = r->units; u; u = u->next)
            freset(u->faction, FFL_SELECT);
        for (u = r->units; u; u = u->next) {
            if (!fval(u->faction, FFL_SELECT)) {
                fset(u->faction, FFL_SELECT);
                add_message(&u->faction->msgs, msg);
            }
        }
        if (!fval(mage->faction, FFL_SELECT)) {
            add_message(&mage->faction->msgs, msg);
        }
        msg_release(msg);
    }
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       'Weg der Baeume'
 * Stufe:      9
 * Kategorie:  Teleport
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Der Druide kann 5*Stufe GE in die astrale Ebene schicken.
 *  Der Druide wird nicht mitteleportiert, es sei denn, er gibt sich
 *  selbst mit an.
 *  Der Zauber funktioniert nur in Waeldern.
 *
 * Syntax: Zauber "Weg der Baeume" <Einheit> ...
 *
 * Flags:
 * (UNITSPELL | SPELLLEVEL | TESTCANSEE)
 */
static int sp_treewalkenter(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    spellparameter *pa = co->par;
    double power = co->force;
    int cast_level = co->level;
    region *rt;
    int remaining_cap;
    int n;
    int erfolg = 0;

    if (getplane(r) != 0) {
        cmistake(mage, co->order, 190, MSG_MAGIC);
        return 0;
    }

    if (!r_isforest(r)) {
        cmistake(mage, co->order, 191, MSG_MAGIC);
        return 0;
    }

    rt = r_standard_to_astral(r);
    if (rt == NULL || is_cursed(rt->attribs, &ct_astralblock)
        || fval(rt->terrain, FORBIDDEN_REGION)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_astralblock", ""));
        return 0;
    }

    remaining_cap = (int)(power * 500);

    /* fuer jede Einheit */
    for (n = 0; n < pa->length; n++) {
        unit *u = pa->param[n]->data.u;
        spllprm *param = pa->param[n];

        if (param->flag & (TARGET_RESISTS | TARGET_NOTFOUND)) {
            continue;
        }

        if (!ucontact(u, mage)) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "feedback_no_contact", "target", u));
        }
        else {
            int w;
            message *m;
            unit *u2;

            if (!can_survive(u, rt)) {
                cmistake(mage, co->order, 231, MSG_MAGIC);
                continue;
            }

            w = weight(u);
            if (remaining_cap - w < 0) {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "fail_tooheavy", "target", u));
                continue;
            }
            remaining_cap = remaining_cap - w;
            move_unit(u, rt, NULL);
            erfolg = cast_level;

            /* Meldungen in der Ausgangsregion */
            for (u2 = r->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);
            m = NULL;
            for (u2 = r->units; u2; u2 = u2->next) {
                if (!fval(u2->faction, FFL_SELECT)) {
                    if (cansee(u2->faction, r, u, 0)) {
                        fset(u2->faction, FFL_SELECT);
                        if (!m)
                            m = msg_message("astral_disappear", "unit", u);
                        r_addmessage(r, u2->faction, m);
                    }
                }
            }
            if (m)
                msg_release(m);

            /* Meldungen in der Zielregion */
            for (u2 = rt->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);
            m = NULL;
            for (u2 = rt->units; u2; u2 = u2->next) {
                if (!fval(u2->faction, FFL_SELECT)) {
                    if (cansee(u2->faction, rt, u, 0)) {
                        fset(u2->faction, FFL_SELECT);
                        if (!m)
                            m = msg_message("astral_appear", "unit", u);
                        r_addmessage(rt, u2->faction, m);
                    }
                }
            }
            if (m)
                msg_release(m);
        }
    }
    return erfolg;
}

/* ------------------------------------------------------------- */
/* Name:       'Sog des Lebens'
 * Stufe:      9
 * Kategorie:  Teleport
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  Der Druide kann 5*Stufe GE aus die astrale Ebene schicken.  Der
 *  Druide wird nicht mitteleportiert, es sei denn, er gibt sich selbst
 *  mit an.
 *  Der Zauber funktioniert nur, wenn die Zielregion ein Wald ist.
 *
 * Syntax: Zauber "Sog des Lebens" <Ziel-X> <Ziel-Y> <Einheit> ...
 *
 * Flags:
 * (UNITSPELL|SPELLLEVEL)
 */
static int sp_treewalkexit(castorder * co)
{
    region *rt;
    region_list *rl, *rl2;
    int tax, tay;
    unit *u, *u2;
    int remaining_cap;
    int n;
    int erfolg = 0;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    double power = co->force;
    spellparameter *pa = co->par;
    int cast_level = co->level;

    if (!is_astral(r)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_astralonly", ""));
        return 0;
    }
    if (is_cursed(r->attribs, &ct_astralblock)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_astralblock", ""));
        return 0;
    }

    remaining_cap = (int)(power * 500);

    if (pa->param[0]->typ != SPP_REGION) {
        report_failure(mage, co->order);
        return 0;
    }

    /* Koordinaten setzen und Region loeschen fuer �berpruefung auf
     * Gueltigkeit */
    rt = pa->param[0]->data.r;
    tax = rt->x;
    tay = rt->y;
    rt = NULL;

    rl = astralregions(r, inhabitable);
    rt = 0;

    rl2 = rl;
    while (rl2) {
        if (rl2->data->x == tax && rl2->data->y == tay) {
            rt = rl2->data;
            break;
        }
        rl2 = rl2->next;
    }
    free_regionlist(rl);

    if (!rt) {
        cmistake(mage, co->order, 195, MSG_MAGIC);
        return 0;
    }

    if (!r_isforest(rt)) {
        cmistake(mage, co->order, 196, MSG_MAGIC);
        return 0;
    }

    /* fuer jede Einheit in der Kommandozeile */
    for (n = 1; n < pa->length; n++) {
        if (pa->param[n]->flag == TARGET_RESISTS
            || pa->param[n]->flag == TARGET_NOTFOUND)
            continue;

        u = pa->param[n]->data.u;

        if (!ucontact(u, mage)) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "feedback_no_contact", "target", u));
        }
        else {
            int w = weight(u);
            if (!can_survive(u, rt)) {
                cmistake(mage, co->order, 231, MSG_MAGIC);
            }
            else if (remaining_cap - w < 0) {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "fail_tooheavy", "target", u));
            }
            else {
                message *m;

                remaining_cap = remaining_cap - w;
                move_unit(u, rt, NULL);
                erfolg = cast_level;

                /* Meldungen in der Ausgangsregion */

                for (u2 = r->units; u2; u2 = u2->next)
                    freset(u2->faction, FFL_SELECT);
                m = NULL;
                for (u2 = r->units; u2; u2 = u2->next) {
                    if (!fval(u2->faction, FFL_SELECT)) {
                        if (cansee(u2->faction, r, u, 0)) {
                            fset(u2->faction, FFL_SELECT);
                            if (!m)
                                m = msg_message("astral_disappear", "unit", u);
                            r_addmessage(r, u2->faction, m);
                        }
                    }
                }
                if (m)
                    msg_release(m);

                /* Meldungen in der Zielregion */

                for (u2 = rt->units; u2; u2 = u2->next)
                    freset(u2->faction, FFL_SELECT);
                m = NULL;
                for (u2 = rt->units; u2; u2 = u2->next) {
                    if (!fval(u2->faction, FFL_SELECT)) {
                        if (cansee(u2->faction, rt, u, 0)) {
                            fset(u2->faction, FFL_SELECT);
                            if (!m)
                                m = msg_message("astral_appear", "unit", u);
                            r_addmessage(rt, u2->faction, m);
                        }
                    }
                }
                if (m)
                    msg_release(m);
            }
        }
    }
    return erfolg;
}

/* ------------------------------------------------------------- */
/* Name:       Heiliger Boden
 * Stufe:      9
 * Kategorie:  perm. Regionszauber
 * Gebiet:     Gwyrrd
 * Wirkung:
 *   Es entstehen keine Untoten mehr, Untote betreten die Region
 *   nicht mehr.
 *
 * ZAUBER "Heiliger Boden"
 * Flags: (0)
 */
static int sp_holyground(castorder * co)
{
    const curse_type *ctype = NULL;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    message *msg = msg_message("sp_holyground_effect", "mage region", mage, r);
    report_spell(mage, r, msg);
    msg_release(msg);

    ctype = &ct_holyground;
    create_curse(mage, &r->attribs, ctype, power * power, 1, zero_effect, 0);

    a_removeall(&r->attribs, &at_deathcount);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Heimstein
 * Stufe:      7
 * Kategorie:  Artefakt
 * Gebiet:     Gwyrrd
 * Wirkung:
 * Die Burg kann nicht mehr durch Donnerbeben oder andere
 * Gebaeudezerstoerenden Sprueche kaputt gemacht werden. Auch
 * schuetzt der Zauber vor Belagerungskatapulten.
 *
 * ZAUBER Heimstein
 * Flags: (0)
 */
static int sp_homestone(castorder * co)
{
    unit *u;
    curse *c;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    double effect;
    message *msg;
    if (!mage->building || !is_building_type(mage->building->type, "castle")) {
        cmistake(mage, co->order, 197, MSG_MAGIC);
        return 0;
    }

    c = create_curse(mage, &mage->building->attribs, &ct_magicwalls,
        force * force, 1, zero_effect, 0);

    if (c == NULL) {
        cmistake(mage, co->order, 206, MSG_MAGIC);
        return 0;
    }

    /* Magieresistenz der Burg erhoeht sich um 50% */
    effect = 50.0F;
    c = create_curse(mage, &mage->building->attribs,
        &ct_magicresistance, force * force, 1, effect, 0);
    c_setflag(c, CURSE_NOAGE);

    /* melden, 1x pro Partei in der Burg */
    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    msg = msg_message("homestone_effect", "mage building", mage, mage->building);
    for (u = r->units; u; u = u->next) {
        if (!fval(u->faction, FFL_SELECT)) {
            fset(u->faction, FFL_SELECT);
            if (u->building == mage->building) {
                r_addmessage(r, u->faction, msg);
            }
        }
    }
    msg_release(msg);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Duerre
 * Stufe:      13
 * Kategorie:  Region, negativ
 * Gebiet:     Gwyrrd
 * Wirkung:
 *  temporaer veraendert sich das Baummaximum und die maximalen Felder in
 *  einer Region auf die Haelfte des normalen.
 *  Die Haelfte der Baeume verdorren und Pferde verdursten.
 *  Arbeiten bringt nur noch 1/4 des normalen Verdienstes
 *
 * Flags:
 * (FARCASTING|REGIONSPELL|TESTRESISTANCE),
 */
static int sp_drought(castorder * co)
{
    curse *c;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    int duration = (int)power + 1;
    message *msg;

    if (fval(r->terrain, SEA_REGION)) {
        cmistake(mage, co->order, 189, MSG_MAGIC);
        /* TODO: vielleicht einen netten Patzer hier? */
        return 0;
    }

    /* melden, 1x pro Partei */
    msg = msg_message("sp_drought_effect", "mage region", mage, r);
    report_spell(mage, r, msg);
    msg_release(msg);

    /* Wenn schon Duerre herrscht, dann setzen wir nur den Power-Level
     * hoch (evtl dauert dann die Duerre laenger).  Ansonsten volle
     * Auswirkungen.
     */
    c = get_curse(r->attribs, &ct_drought);
    if (c) {
        c->vigour = MAX(c->vigour, power);
        c->duration = MAX(c->duration, (int)power);
    }
    else {
        double effect = 4.0;
        /* Baeume und Pferde sterben */
        rsettrees(r, 2, rtrees(r, 2) / 2);
        rsettrees(r, 1, rtrees(r, 1) / 2);
        rsettrees(r, 0, rtrees(r, 0) / 2);
        rsethorses(r, rhorses(r) / 2);

        create_curse(mage, &r->attribs, &ct_drought, power, duration, effect,
            0);
    }
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Bergwaechter
 * Stufe:      9
 * Gebiet:     Gwyrrd
 * Kategorie:  Beschwoerung, negativ
 *
 * Wirkung:
 * Erschafft in Bergen oder Gletschern einen Waechter, der durch bewachen
 * den Eisen/Laen-Abbau fuer nicht-Allierte verhindert.  Bergwaechter
 * verhindern auch Abbau durch getarnte/unsichtbare Einheiten und lassen
 * sich auch durch Belagerungen nicht aufhalten.
 *
 * (Ansonsten in economic.c:manufacture() entsprechend anpassen).
 *
 * Faehigkeiten (factypes.c): 50% Magieresistenz, 25 HP, 4d4 Schaden,
 *   4 Ruestung (=PP)
 * Flags:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
static int sp_ironkeeper(castorder * co)
{
    unit *keeper;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    message *msg;

    if (r->terrain != newterrain(T_MOUNTAIN)
        && r->terrain != newterrain(T_GLACIER)) {
        report_failure(mage, co->order);
        return 0;
    }

    keeper =
        create_unit(r, mage->faction, 1, get_race(RC_IRONKEEPER), 0, NULL, mage);

    /*keeper->age = cast_level + 2; */
    setstatus(keeper, ST_AVOID);  /* kaempft nicht */
    setguard(keeper, true);
    fset(keeper, UFL_ISNEW);
    /* Parteitarnen, damit man nicht sofort wei�, wer dahinter steckt */
    if (rule_stealth_anon()) {
        fset(keeper, UFL_ANON_FACTION);
    }

    {
        trigger *tkill = trigger_killunit(keeper);
        add_trigger(&keeper->attribs, "timer", trigger_timeout(cast_level + 2,
            tkill));
    }

    msg = msg_message("summon_effect", "mage amount race", mage, 1, u_race(keeper));
    r_addmessage(r, NULL, msg);
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Sturmwind - Beschwoere einen Sturmelementar
 * Stufe:      6
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 * Verdoppelt Geschwindigkeit aller angegebener Schiffe fuer diese
 * Runde.  Kombinierbar mit "Guenstige Winde", aber nicht mit
 * "Luftschiff".
 *
 * Anstelle des alten ship->enchanted benutzen wir einen kurzfristigen
 * Curse.  Das ist zwar ein wenig aufwendiger, aber weitaus flexibler
 * und erlaubt es zB, die Dauer spaeter problemlos zu veraendern.
 *
 * Flags:
 *  (SHIPSPELL|ONSHIPCAST|OCEANCASTABLE|TESTRESISTANCE)
 */

static int sp_stormwinds(castorder * co)
{
    ship *sh;
    unit *u;
    int erfolg = 0;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    double power = co->force;
    spellparameter *pa = co->par;
    int n, force = (int)power;
    message *m = NULL;

    /* melden vorbereiten */
    freset(mage->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);

    for (n = 0; n < pa->length; n++) {
        if (force <= 0)
            break;

        if (pa->param[n]->flag == TARGET_RESISTS
            || pa->param[n]->flag == TARGET_NOTFOUND)
            continue;

        sh = pa->param[n]->data.sh;

        /* mit C_SHIP_NODRIFT haben wir kein Problem */
        if (is_cursed(sh->attribs, &ct_flyingship)) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "error_spell_on_flying_ship", "ship", sh))
                continue;
        }
        if (is_cursed(sh->attribs, &ct_shipspeedup)) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "error_spell_on_ship_already", "ship", sh))
                continue;
        }

        /* Duration = 1, nur diese Runde */
        create_curse(mage, &sh->attribs, &ct_stormwind, power, 1,
            zero_effect, 0);
        /* Da der Spruch nur diese Runde wirkt wird er nie im Report
         * erscheinen */
        erfolg++;
        force--;

        /* melden vorbereiten: */
        for (u = r->units; u; u = u->next) {
            if (u->ship == sh) {
                /* nur den Schiffsbesatzungen! */
                fset(u->faction, FFL_SELECT);
            }
        }
    }
    if (erfolg < pa->length) {
        ADDMSG(&mage->faction->msgs, msg_message("stormwinds_reduced",
            "unit ships maxships", mage, erfolg, pa->length));
    }
    /* melden, 1x pro Partei auf Schiff und fuer den Magier */
    fset(mage->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next) {
        if (fval(u->faction, FFL_SELECT)) {
            freset(u->faction, FFL_SELECT);
            if (erfolg > 0) {
                if (!m) {
                    m = msg_message("stormwinds_effect", "unit", mage);
                }
                r_addmessage(r, u->faction, m);
            }
        }
    }
    if (m)
        msg_release(m);
    return erfolg;
}

/* ------------------------------------------------------------- */
/* Name:       Donnerbeben
 * Stufe:      6
 * Gebiet:     Gwyrrd
 *
 * Wirkung:
 * Zerstoert Stufe*10 "Steineinheiten" aller Gebaeude der Region, aber nie
 * mehr als 25% des gesamten Gebaeudes (aber natuerlich mindestens ein
 * Stein).
 *
 * Flags:
 *  (FARCASTING|REGIONSPELL|TESTRESISTANCE)
 */
static int sp_earthquake(castorder * co)
{
    int kaputt;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    message *msg;
    building **blist = &r->buildings;

    while (*blist) {
        building *burg = *blist;

        if (burg->size != 0 && !is_cursed(burg->attribs, &ct_magicwalls)) {
            /* Magieresistenz */
            if (!target_resists_magic(mage, burg, TYP_BUILDING, 0)) {
                kaputt = MIN(10 * cast_level, burg->size / 4);
                kaputt = MAX(kaputt, 1);
                burg->size -= kaputt;
                if (burg->size == 0) {
                    /* TODO: sollten die Insassen nicht Schaden nehmen? */
                    remove_building(blist, burg);
                }
            }
        }
        if (*blist == burg)
            blist = &burg->next;
    }

    /* melden, 1x pro Partei */
    msg = msg_message("earthquake_effect", "mage region", mage, r);
    r_addmessage(r, NULL, msg);
    msg_release(msg);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* CHAOS / M_DRAIG / Draig */
/* ------------------------------------------------------------- */
void patzer_peasantmob(const castorder * co)
{
    int anteil = 6, n;
    unit *u;
    attrib *a;
    region *r;
    unit *mage = co->magician.u;

    r = mage->region->land ? mage->region : co_get_region(co);

    if (r->land) {
        faction *f = get_monsters();
        const struct locale *lang = f->locale;
        message *msg;

        anteil += rng_int() % 4;
        n = rpeasants(r) * anteil / 10;
        rsetpeasants(r, rpeasants(r) - n);
        assert(rpeasants(r) >= 0);

        u =
            create_unit(r, f, n, get_race(RC_PEASANT), 0, LOC(f->locale, "angry_mob"),
                NULL);
        fset(u, UFL_ISNEW);
        addlist(&u->orders, create_order(K_GUARD, lang, NULL));
        set_order(&u->thisorder, default_order(lang));
        a = a_new(&at_unitdissolve);
        a->data.ca[0] = 1;          /* An rpeasants(r). */
        a->data.ca[1] = 10;         /* 10% */
        a_add(&u->attribs, a);
        a_add(&u->attribs, make_hate(mage));

        msg = msg_message("mob_warning", "");
        r_addmessage(r, NULL, msg);
        msg_release(msg);
    }
    return;
}

/* ------------------------------------------------------------- */
/* Name:       Waldbrand
 * Stufe:      10
 * Kategorie:  Region, negativ
 * Gebiet:     Draig
 * Wirkung:
 * Vernichtet 10-80% aller Baeume in der Region.  Kann sich auf benachbarte
 * Regionen ausbreiten, wenn diese (stark) bewaldet sind.  Fuer jeweils
 * 10 verbrannte Baeume in der Startregion gibts es eine 1%-Chance, dass
 * sich das Feuer auf stark bewaldete Nachbarregionen ausdehnt, auf
 * bewaldeten mit halb so hoher Wahrscheinlichkeit.  Dort verbrennen
 * dann prozentual halbsoviele bzw ein viertel soviele Baeume wie in der
 * Startregion.
 *
 * Im Extremfall: 1250 Baeume in Region, 80% davon verbrennen (1000).
 * Dann breitet es sich mit 100% Chance in stark bewaldete Regionen
 * aus, mit 50% in bewaldete.  Dort verbrennen dann 40% bzw 20% der Baeume.
 * Weiter als eine Nachbarregion breitet sich dass Feuer nicht aus.
 *
 * Sinn: Ein Feuer in einer "stark bewaldeten" Wueste hat so trotzdem kaum
 * eine Chance, sich weiter auszubreiten, waehrend ein Brand in einem Wald
 * sich fast mit Sicherheit weiter ausbreitet.
 *
 * Flags:
 * (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
static int sp_forest_fire(castorder * co)
{
    unit *u;
    region *nr;
    direction_t i;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double probability;
    double percentage = (rng_int() % 8 + 1) * 0.1;        /* 10 - 80% */
    message *msg;

    int vernichtet_schoesslinge = (int)(rtrees(r, 1) * percentage);
    int destroyed = (int)(rtrees(r, 2) * percentage);

    if (destroyed < 1) {
        cmistake(mage, co->order, 198, MSG_MAGIC);
        return 0;
    }

    rsettrees(r, 2, rtrees(r, 2) - destroyed);
    rsettrees(r, 1, rtrees(r, 1) - vernichtet_schoesslinge);
    probability = destroyed * 0.001;      /* Chance, dass es sich ausbreitet */

    /* melden, 1x pro Partei */
    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    msg =
        msg_message("forestfire_effect", "mage region amount", mage, r,
            destroyed + vernichtet_schoesslinge);
    r_addmessage(r, NULL, msg);
    add_message(&mage->faction->msgs, msg);
    msg_release(msg);

    for (i = 0; i < MAXDIRECTIONS; i++) {
        nr = rconnect(r, i);
        assert(nr);
        destroyed = 0;
        vernichtet_schoesslinge = 0;

        if (rtrees(nr, 2) + rtrees(nr, 1) >= 800) {
            if (chance(probability)) {
                destroyed = (int)(rtrees(nr, 2) * percentage / 2);
                vernichtet_schoesslinge = (int)(rtrees(nr, 1) * percentage / 2);
            }
        }
        else if (rtrees(nr, 2) + rtrees(nr, 1) >= 600) {
            if (chance(probability / 2)) {
                destroyed = (int)(rtrees(nr, 2) * percentage / 4);
                vernichtet_schoesslinge = (int)(rtrees(nr, 1) * percentage / 4);
            }
        }

        if (destroyed > 0 || vernichtet_schoesslinge > 0) {
            message *m = msg_message("forestfire_spread", "region next trees",
                r, nr, destroyed + vernichtet_schoesslinge);

            add_message(&r->msgs, m);
            add_message(&mage->faction->msgs, m);
            msg_release(m);

            rsettrees(nr, 2, rtrees(nr, 2) - destroyed);
            rsettrees(nr, 1, rtrees(nr, 1) - vernichtet_schoesslinge);
        }
    }
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Chaosfluch
 * Stufe:      5
 * Gebiet:     Draig
 * Kategorie:  (Antimagie) Kraftreduzierer, Einheit, negativ
 * Wirkung:
 *  Auf einen Magier gezaubert verhindert/erschwert dieser Chaosfluch
 *  das Zaubern. Patzer werden warscheinlicher.
 *  Jeder Zauber muss erst gegen den Wiederstand des Fluchs gezaubert
 *  werden und schwaecht dessen Antimagiewiederstand um 1.
 *  Wirkt MAX(Stufe(Magier) - Stufe(Ziel), rand(3)) Wochen
 * Patzer:
 *  Magier wird selbst betroffen
 *
 * Flags:
 *  (UNITSPELL | SPELLLEVEL | TESTCANSEE | TESTRESISTANCE)
 *
 */
static int sp_fumblecurse(castorder * co)
{
    unit *target;
    int rx, sx;
    int duration;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    double effect;
    curse *c;
    spellparameter *pa = co->par;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    target = pa->param[0]->data.u;

    rx = rng_int() % 3;
    sx = cast_level - effskill(target, SK_MAGIC, 0);
    duration = MAX(sx, rx) + 1;

    effect = force / 2;
    c = create_curse(mage, &target->attribs, &ct_fumble,
        force, duration, effect, 0);
    if (c == NULL) {
        report_failure(mage, co->order);
        return 0;
    }

    ADDMSG(&target->faction->msgs, msg_message("fumblecurse", "unit region",
        target, target->region));

    return cast_level;
}

void patzer_fumblecurse(const castorder * co)
{
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = (cast_level / 2) + 1;
    double effect;
    curse *c;

    effect = force / 2;
    c = create_curse(mage, &mage->attribs, &ct_fumble, force,
        duration, effect, 0);
    if (c != NULL) {
        ADDMSG(&mage->faction->msgs, msg_message("magic_fumble",
            "unit region command", mage, mage->region, co->order));
    }
    return;
}

/* ------------------------------------------------------------- */
/* Name:       Drachenruf
 * Stufe:      11
 * Gebiet:     Draig
 * Kategorie:  Monster, Beschwoerung, negativ
 *
 * Wirkung:
 *  In einer Wueste, Sumpf oder Gletscher gezaubert kann innerhalb der
 *  naechsten 6 Runden ein bis 6 Dracheneinheiten bis Groe�e Wyrm
 *  entstehen.
 *
 *  Mit Stufe 12-15 erscheinen Jung- oder normaler Drachen, mit Stufe
 *  16+ erscheinen normale Drachen oder Wyrme.
 *
 * Flag:
 *  (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */

static int sp_summondragon(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    unit *u;
    int cast_level = co->level;
    double power = co->force;
    region_list *rl, *rl2;
    faction *f;
    int time;
    int number;
    const race *race;

    f = get_monsters();

    if (r->terrain != newterrain(T_SWAMP) && r->terrain != newterrain(T_DESERT)
        && r->terrain != newterrain(T_GLACIER)) {
        report_failure(mage, co->order);
        return 0;
    }

    for (time = 1; time < 7; time++) {
        if (rng_int() % 100 < 25) {
            switch (rng_int() % 3) {
            case 0:
                race = get_race(RC_WYRM);
                number = 1;
                break;

            case 1:
                race = get_race(RC_DRAGON);
                number = 2;
                break;

            case 2:
            default:
                race = get_race(RC_FIREDRAGON);
                number = 6;
                break;
            }
            {
                trigger *tsummon = trigger_createunit(r, f, race, number);
                add_trigger(&r->attribs, "timer", trigger_timeout(time, tsummon));
            }
        }
    }

    rl = all_in_range(r, (short)power, NULL);

    for (rl2 = rl; rl2; rl2 = rl2->next) {
        region *r2 = rl2->data;
        for (u = r2->units; u; u = u->next) {
            if (u_race(u) == get_race(RC_WYRM) || u_race(u) == get_race(RC_DRAGON)) {
                attrib *a = a_find(u->attribs, &at_targetregion);
                if (!a) {
                    a = a_add(&u->attribs, make_targetregion(r));
                }
                else {
                    a->data.v = r;
                }
            }
        }
    }

    ADDMSG(&mage->faction->msgs, msg_message("summondragon",
        "unit region command target", mage, mage->region, co->order, r));

    free_regionlist(rl);
    return cast_level;
}

static int sp_firewall(castorder * co)
{
    connection *b;
    wall_data *fd;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    spellparameter *pa = co->par;
    direction_t dir;
    region *r2;

    dir = get_direction(pa->param[0]->data.xs, mage->faction->locale);
    if (dir < MAXDIRECTIONS && dir != NODIRECTION) {
        r2 = rconnect(r, dir);
    }
    else {
        report_failure(mage, co->order);
        return 0;
    }

    if (!r2 || r2 == r) {
        report_failure(mage, co->order);
        return 0;
    }

    b = get_borders(r, r2);
    while (b != NULL) {
        if (b->type == &bt_firewall)
            break;
        b = b->next;
    }
    if (b == NULL) {
        b = new_border(&bt_firewall, r, r2);
        fd = (wall_data *)b->data.v;
        fd->force = (int)(force / 2 + 0.5);
        fd->mage = mage;
        fd->active = false;
        fd->countdown = cast_level + 1;
    }
    else {
        fd = (wall_data *)b->data.v;
        fd->force = (int)MAX(fd->force, force / 2 + 0.5);
        fd->countdown = MAX(fd->countdown, cast_level + 1);
    }

    /* melden, 1x pro Partei */
    {
        message *seen = msg_message("firewall_effect", "mage region", mage, r);
        message *unseen = msg_message("firewall_effect", "mage region", NULL, r);
        report_effect(r, mage, seen, unseen);
        msg_release(seen);
        msg_release(unseen);
    }

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Unheilige Kraft
 * Stufe:      10
 * Gebiet:     Draig
 * Kategorie:  Untote Einheit, positiv
 *
 * Wirkung:
 *  transformiert (Stufe)W10 Untote in ihre staerkere Form
 *
 *
 * Flag:
 *   (SPELLLEVEL | TESTCANSEE)
 */

static const race *unholy_race(const race *rc) {
    static int cache;
    static const race * rc_skeleton, *rc_zombie, *rc_ghoul;
    if (rc_changed(&cache)) {
        rc_skeleton = get_race(RC_SKELETON);
        rc_zombie = get_race(RC_ZOMBIE);
        rc_ghoul = get_race(RC_GHOUL);
    }
    if (rc == rc_skeleton) {
        return get_race(RC_SKELETON_LORD);
    }
    if (rc == rc_zombie) {
        return get_race(RC_ZOMBIE_LORD);
    }
    if (rc == rc_ghoul) {
        return get_race(RC_GHOUL_LORD);
    }
    return NULL;
}

static int sp_unholypower(castorder * co)
{
    region * r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;
    int i;
    int n;
    int wounds;

    n = dice((int)co->force, 10);

    for (i = 0; i < pa->length && n > 0; i++) {
        const race *target_race;
        unit *u;

        if (pa->param[i]->flag == TARGET_RESISTS
            || pa->param[i]->flag == TARGET_NOTFOUND)
            continue;

        u = pa->param[i]->data.u;

        target_race = unholy_race(u_race(u));
        if (!target_race) {
            cmistake(mage, co->order, 284, MSG_MAGIC);
            continue;
        }
        /* Untote heilen nicht, darum den neuen Untoten maximale hp geben
         * und vorhandene Wunden abziehen */
        wounds = unit_max_hp(u) * u->number - u->hp;

        if (u->number <= n) {
            n -= u->number;
            u->irace = NULL;
            u_setrace(u, target_race);
            u->hp = unit_max_hp(u) * u->number - wounds;
            ADDMSG(&r->msgs, msg_message("unholypower_effect",
                "mage target race", mage, u, target_race));
        }
        else {
            unit *un;

            /* Wird hoffentlich niemals vorkommen. Es gibt im Source
             * vermutlich eine ganze Reihe von Stellen, wo das nicht
             * korrekt abgefangen wird. Besser (aber nicht gerade einfach)
             * waere es, eine solche Konstruktion irgendwie zu kapseln. */
            if (fval(u, UFL_LOCKED) || fval(u, UFL_HUNGER)
                || is_cursed(u->attribs, &ct_slavery)) {
                cmistake(mage, co->order, 74, MSG_MAGIC);
                continue;
            }
            /* Verletzungsanteil der transferierten Personen berechnen */
            wounds = wounds * n / u->number;

            un = create_unit(r, u->faction, 0, target_race, 0, NULL, u);
            transfermen(u, un, n);
            un->hp = unit_max_hp(un) * n - wounds;
            ADDMSG(&r->msgs, msg_message("unholypower_limitedeffect",
                "mage target race amount", mage, u, target_race, n));
            n = 0;
        }
    }

    return cast_level;
}

static int change_hitpoints(unit * u, int value)
{
    int hp = u->hp;

    hp += value;

    /* Jede Person ben�tigt mindestens 1 HP */
    if (hp < u->number) {
        if (hp < 0) {               /* Einheit tot */
            hp = 0;
        }
        scale_number(u, hp);
    }
    u->hp = hp;
    return hp;
}

static int dc_age(struct curse *c)
/* age returns 0 if the attribute needs to be removed, !=0 otherwise */
{
    region *r = (region *)c->data.v;
    unit **up;
    unit *mage = c->magician;

    if (r == NULL || mage == NULL || mage->number == 0) {
        /* if the mage disappears, so does the spell. */
        return AT_AGE_REMOVE;
    }

    up = &r->units;
    if (curse_active(c))
        while (*up != NULL) {
            unit *u = *up;
            int hp;
            variant dmg = frac_make(u->number, 1);
            double damage = c->effect;

            if (u->number <= 0 || target_resists_magic(mage, u, TYP_UNIT, 0)) {
                up = &u->next;
                continue;
            }

            /* Reduziert durch Magieresistenz */
            dmg = frac_mul(dmg, frac_sub(frac_make(1,1), magic_resistance(u)));
            damage *= dmg.sa[0];
            damage /= dmg.sa[1];
            hp = change_hitpoints(u, -(int)damage);

            ADDMSG(&u->faction->msgs, msg_message((hp > 0) ? "poison_damage" : "poison_death", "region unit", r, u));
            if (*up == u)
                up = &u->next;
        }

    return AT_AGE_KEEP;
}

static struct curse_type ct_deathcloud = {
    "deathcloud", CURSETYP_REGION, 0, NO_MERGE, cinfo_simple, NULL, NULL, NULL,
    NULL, dc_age
};

static curse *mk_deathcloud(unit * mage, region * r, double force, int duration)
{
    double effect;
    curse *c;

    effect = force / 2;
    c =
        create_curse(mage, &r->attribs, &ct_deathcloud, force, duration, effect, 0);
    c->data.v = r;
    return c;
}

#define COMPAT_DEATHCLOUD
#ifdef COMPAT_DEATHCLOUD
static int dc_read_compat(struct attrib *a, void *target, gamedata *data)
/* return AT_READ_OK on success, AT_READ_FAIL if attrib needs removal */
{
    struct storage *store = data->store;
    region *r = NULL;
    unit *u;
    int id;
    int duration;
    float strength;
    int rx, ry;

    UNUSED_ARG(a);
    UNUSED_ARG(target);
    READ_INT(store, &duration);
    READ_FLT(store, &strength);
    READ_INT(store, &id);
    u = findunit(id);

    /* this only affects really old data. no need to change: */
    READ_INT(store, &rx);
    READ_INT(store, &ry);
    r = findregion(rx, ry);

    if (r != NULL) {
        double effect;
        curse *c;

        effect = strength;
        c = create_curse(u, &r->attribs, &ct_deathcloud, strength * 2, duration,
            effect, 0);
        c->data.v = r;
        if (u == NULL) {
            ur_add(id, &c->magician, NULL);
        }
    }
    return AT_READ_FAIL;          /* we don't care for the attribute. */
}
#endif

/* ------------------------------------------------------------- */
/* Name:       Todeswolke
* Stufe:      11
* Gebiet:     Draig
* Kategorie:  Region, negativ
*
* Wirkung:
*   Personen in der Region verlieren stufe/2 Trefferpunkte pro Runde.
*   Dauer force/2
*   Wirkt gegen MR
*   Ruestung wirkt nicht
* Patzer:
*   Magier geraet in den Staub und verliert zufaellige Zahl von HP bis
*   auf MAX(hp,2)
* Besonderheiten:
*   Nicht als curse implementiert, was schlecht ist - man kann dadurch
*   kein dispell machen. Wegen fix unter Zeitdruck erstmal nicht zu
*   aendern...
* Missbrauchsmoeglichkeit:
*   Hat der Magier mehr HP als Rasse des Feindes (extrem: Daemon/Goblin)
*   so kann er per Farcasting durch mehrmaliges Zaubern eine
*   Nachbarregion ausloeschen. Darum sollte dieser Spruch nur einmal auf
*   eine Region gelegt werden koennen.
*
* Flag:
*   (FARCASTING | REGIONSPELL | TESTRESISTANCE)
*/

static int sp_deathcloud(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    attrib *a = r->attribs;
    unit *u;

    while (a) {
        if (a->type == &at_curse) {
            curse *c = a->data.v;
            if (c->type == &ct_deathcloud) {
                report_failure(mage, co->order);
                return 0;
            }
            a = a->next;
        }
        else
            a = a->nexttype;
    }

    mk_deathcloud(mage, r, co->force, co->level);

    /* melden, 1x pro Partei */
    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next) {
        if (!fval(u->faction, FFL_SELECT)) {
            fset(u->faction, FFL_SELECT);
            ADDMSG(&u->faction->msgs, msg_message("deathcloud_effect",
                "mage region", cansee(u->faction, r, mage, 0) ? mage : NULL, r));
        }
    }

    if (!fval(mage->faction, FFL_SELECT)) {
        ADDMSG(&mage->faction->msgs, msg_message("deathcloud_effect",
            "mage region", mage, r));
    }

    return co->level;
}

void patzer_deathcloud(castorder * co)
{
    unit *mage = co->magician.u;
    int hp = (mage->hp - 2);

    change_hitpoints(mage, -rng_int() % hp);

    ADDMSG(&mage->faction->msgs, msg_message("magic_fumble",
        "unit region command", mage, mage->region, co->order));

    return;
}

/* ------------------------------------------------------------- */
/* Name:      Pest
 * Stufe:     7
 * Gebiet:    Draig
 * Wirkung:
 *  ruft eine Pest in der Region hervor.
 * Flags:
 *  (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 * Syntax: ZAUBER [REGION x y] "Pest"
 */
static int sp_plague(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;

    plagues(r);

    ADDMSG(&mage->faction->msgs, msg_message("plague_spell",
        "region mage", r, mage));
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Beschwoere Schattendaemon
 * Stufe:      8
 * Gebiet:     Draig
 * Kategorie:  Beschwoerung, positiv
 * Wirkung:
 *  Der Magier beschwoert Stufe^2 Schattendaemonen.
 *  Schattendaemonen haben Tarnung = (Magie_Magier+ Tarnung_Magier)/2 und
 *  Wahrnehmung 1. Sie haben einen Attacke-Bonus von 8, einen
 *  Verteidigungsbonus von 11 und machen 2d3 Schaden.  Sie entziehen bei
 *  einem Treffer dem Getroffenen einen Attacke- oder
 *  Verteidigungspunkt.  (50% Chance.) Sie haben 25 Hitpoints und
 *  Ruestungsschutz 3.
 * Flag:
 *  (SPELLLEVEL)
 */
static int sp_summonshadow(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    unit *u;
    int val, number = (int)(force * force);

    u = create_unit(r, mage->faction, number, get_race(RC_SHADOW), 0, NULL, mage);

    /* Bekommen Tarnung = (Magie+Tarnung)/2 und Wahrnehmung 1. */
    val = get_level(mage, SK_MAGIC) + get_level(mage, SK_STEALTH);

    set_level(u, SK_STEALTH, val);
    set_level(u, SK_PERCEPTION, 1);

    ADDMSG(&mage->faction->msgs, msg_message("summonshadow_effect",
        "mage number", mage, number));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Beschwoere Schattenmeister
 * Stufe:      12
 * Gebiet:     Draig
 * Kategorie:  Beschwoerung, positiv
 * Wirkung:
 *  Diese hoeheren Schattendaemonen sind erheblich gefaehrlicher als die
 *  einfachen Schattendaemonen.  Sie haben Tarnung entsprechend dem
 *  Magietalent des Beschwoerer-1 und Wahrnehmung 5, 75 HP,
 *  Ruestungsschutz 4, Attacke-Bonus 11 und Verteidigungsbonus 13, machen
 *  bei einem Treffer 2d4 Schaden, entziehen einen Staerkepunkt und
 *  entziehen 5 Talenttage in einem zufaelligen Talent.
 *  Stufe^2 Daemonen.
 *
 * Flag:
 *  (SPELLLEVEL)
 * */
static int sp_summonshadowlords(castorder * co)
{
    unit *u;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int amount = (int)(force * force);

    u = create_unit(r, mage->faction, amount, get_race(RC_SHADOWLORD), 0,
        NULL, mage);

    /* Bekommen Tarnung = Magie und Wahrnehmung 5. */
    set_level(u, SK_STEALTH, get_level(mage, SK_MAGIC));
    set_level(u, SK_PERCEPTION, 5);

    ADDMSG(&mage->faction->msgs, msg_message("summon_effect", "mage amount race",
        mage, amount, u_race(u)));
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Chaossog
 * Stufe:      14
 * Gebiet:     Draig
 * Kategorie:  Teleport
 * Wirkung:
 *  Durch das Opfern von 200 Bauern kann der Chaosmagier ein Tor zur
 *  astralen Welt oeffnen. Das Tor kann im Folgemonat verwendet werden,
 *  es loest sich am Ende des Folgemonats auf.
 *
 * Flag:  (0)
 */
static int sp_chaossuction(castorder * co)
{
    region *rt;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;

    if (rplane(r)) {
        /* Der Zauber funktioniert nur in der materiellen Welt. */
        cmistake(mage, co->order, 190, MSG_MAGIC);
        return 0;
    }

    rt = r_standard_to_astral(r);

    if (rt == NULL || fval(rt->terrain, FORBIDDEN_REGION)) {
        /* Hier gibt es keine Verbindung zur astralen Welt. */
        cmistake(mage, co->order, 216, MSG_MAGIC);
        return 0;
    }
    else if (is_cursed(rt->attribs, &ct_astralblock)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_astralblock", ""));
        return 0;
    }

    /* TODO: implement with a building */
    create_special_direction(r, rt, 2, "vortex_desc", "vortex", false);
    create_special_direction(rt, r, 2, "vortex_desc", "vortex", false);
    new_border(&bt_chaosgate, r, rt);

    add_message(&r->msgs, msg_message("chaosgate_effect_1", "mage", mage));
    add_message(&rt->msgs, msg_message("chaosgate_effect_2", ""));
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Magic Boost - Gabe des Chaos
 * Stufe:      3
 * Gebiet:     Draig
 * Kategorie:  Einheit, positiv
 *
 * Wirkung:
 *   Erhoeht die maximalen Magiepunkte und die monatliche Regeneration auf
 *   das doppelte. Dauer: 4 Wochen Danach sinkt beides auf die Haelfte des
 *   normalen ab.
 * Dauer: 6 Wochen
 * Patzer:
 *   permanenter Stufen- (Talenttage), Regenerations- oder maxMP-Verlust
 * Besonderheiten:
 *   Patzer koennen waehrend der Zauberdauer haeufiger auftreten derzeit
 *   +10%
 *
 * Flag:
 *  (ONSHIPCAST)
 */

static int sp_magicboost(castorder * co)
{
    curse *c;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    double effect;
    trigger *tsummon;

    /* fehler, wenn schon ein boost */
    if (is_cursed(mage->attribs, &ct_magicboost)) {
        report_failure(mage, co->order);
        return 0;
    }

    effect = 6;
    create_curse(mage, &mage->attribs, &ct_magicboost, power, 10, effect, 1);
    /* one aura boost with 200% aura now: */
    effect = 200;
    c = create_curse(mage, &mage->attribs, &ct_auraboost, power, 4, effect, 1);

    /* and one aura boost with 50% aura in 5 weeks: */
    tsummon = trigger_createcurse(mage, mage, &ct_auraboost, power, 6, 50, 1);
    add_trigger(&mage->attribs, "timer", trigger_timeout(5, tsummon));

    ADDMSG(&mage->faction->msgs, msg_message("magicboost_effect",
        "unit region command", c->magician, mage->region, co->order));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       kleines Blutopfer
 * Stufe:      4
 * Gebiet:     Draig
 * Kategorie:  Einheit, positiv
 *
 * Wirkung:
 *   Hitpoints to Aura:
 *   skill < 8  = 4:1
 *   skill < 12 = 3:1
 *   skill < 15 = 2:1
 *   skill < 18 = 1:2
 *   skill >    = 2:1
 * Patzer:
 *   permanenter HP verlust
 *
 * Flag:
 *  (ONSHIPCAST)
 */
static int sp_bloodsacrifice(castorder * co)
{
    unit *mage = co->magician.u;
    int cast_level = co->level;
    int aura;
    int skill = effskill(mage, SK_MAGIC, 0);
    int hp = (int)(co->force * 8);

    if (hp <= 0) {
        report_failure(mage, co->order);
        return 0;
    }

    aura = lovar(hp);

    if (skill < 8) {
        aura /= 4;
    }
    else if (skill < 12) {
        aura /= 3;
    }
    else if (skill < 15) {
        aura /= 2;
        /* von 15 bis 17 ist hp = aura */
    }
    else if (skill > 17) {
        aura *= 2;
    }

    if (aura <= 0) {
        report_failure(mage, co->order);
        return 0;
    }

    /* sicherheitshalber gibs hier einen HP gratis. sonst schaffen es
     * garantiert ne ganze reihe von leuten ihren Magier damit umzubringen */
    mage->hp++;
    change_spellpoints(mage, aura);
    ADDMSG(&mage->faction->msgs,
        msg_message("sp_bloodsacrifice_effect",
            "unit region command amount", mage, mage->region, co->order, aura));
    return cast_level;
}

/** gives a summoned undead unit some base skills.
 */
static void skill_summoned(unit * u, int level)
{
    if (level > 0) {
        const race *rc = u_race(u);
        skill_t sk;
        for (sk = 0; sk != MAXSKILLS; ++sk) {
            if (rc->bonus[sk] > 0) {
                set_level(u, sk, level);
            }
        }
        if (rc->bonus[SK_STAMINA]) {
            u->hp = unit_max_hp(u) * u->number;
        }
    }
}

/* ------------------------------------------------------------- */
/* Name:       Totenruf - Maechte des Todes
 * Stufe:      6
 * Gebiet:     Draig
 * Kategorie:  Beschwoerung, positiv
 * Flag:       FARCASTING
 * Wirkung:
 *   Untote aus deathcounther ziehen, bis Stufe*10 Stueck
 *
 * Patzer:
 *   Erzeugt Monsteruntote
 */
static int sp_summonundead(castorder * co)
{
    int undead;
    unit *u;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    int force = (int)(co->force * 10);
    const race *race = get_race(RC_SKELETON);

    if (!r->land || deathcount(r) == 0) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "error_nograves",
            "target", r));
        return 0;
    }

    undead = MIN(deathcount(r), 2 + lovar(force));

    if (cast_level <= 8) {
        race = get_race(RC_SKELETON);
    }
    else if (cast_level <= 12) {
        race = get_race(RC_ZOMBIE);
    }
    else {
        race = get_race(RC_GHOUL);
    }

    u = create_unit(r, mage->faction, undead, race, 0, NULL, mage);
    make_undead_unit(u);
    skill_summoned(u, cast_level / 2);

    ADDMSG(&mage->faction->msgs, msg_message("summonundead_effect_1",
        "mage region amount", mage, r, undead));
    ADDMSG(&r->msgs, msg_message("summonundead_effect_2", "mage region", mage,
        r));
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Astraler Sog
 * Stufe:      9
 * Gebiet:     Draig
 * Kategorie:  Region, negativ
 * Wirkung:
 *   Allen Magier in der betroffenen Region wird eine Teil ihrer
 *   Magischen Kraft in die Gefilde des Chaos entzogen Jeder Magier im
 *   Einflussbereich verliert Stufe(Zaubernden)*5% seiner Magiepunkte.
 *   Keine Regeneration in der Woche (fehlt noch)
 *
 * Flag:
 *   (REGIONSPELL | TESTRESISTANCE)
 */

static int sp_auraleak(castorder * co)
{
    int lost_aura;
    double lost;
    unit *u;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    message *msg;

    lost = MIN(0.95, cast_level * 0.05);

    for (u = r->units; u; u = u->next) {
        if (is_mage(u)) {
            /* Magieresistenz Einheit?  Bei gegenerischen Magiern nur sehr
             * geringe Chance auf Erfolg wg erhoehter MR, wuerde Spruch sinnlos
             * machen */
            lost_aura = (int)(get_spellpoints(u) * lost);
            change_spellpoints(u, -lost_aura);
        }
    }
    msg = msg_message("cast_auraleak_effect", "mage region", mage, r);
    r_addmessage(r, NULL, msg);
    msg_release(msg);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* BARDE  - CERDDOR*/
/* ------------------------------------------------------------- */
/* ------------------------------------------------------------- */
/* Name:       Magie analysieren - Gebaeude, Schiffe, Region
 * Name:       Lied des Ortes analysieren
 * Stufe:      8
 * Gebiet:     Cerddor
 *
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstaerke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 *
 * Flag:
 *  (SPELLLEVEL|ONSHIPCAST)
 */
static int sp_analysesong_obj(castorder * co)
{
    int obj;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    spellparameter *pa = co->par;

    obj = pa->param[0]->typ;

    switch (obj) {
    case SPP_REGION:
        magicanalyse_region(r, mage, force);
        break;

    case SPP_BUILDING:
    {
        building *b = pa->param[0]->data.b;
        magicanalyse_building(b, mage, force);
        break;
    }
    case SPP_SHIP:
    {
        ship *sh = pa->param[0]->data.sh;
        magicanalyse_ship(sh, mage, force);
        break;
    }
    default:
        /* Syntax fehlerhaft */
        return 0;
    }

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Gesang des Lebens analysieren
 * Name:     Magie analysieren - Unit
 * Stufe:   5
 * Gebiet:   Cerddor
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstaerke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 *
 * Flag:
 *  (UNITSPELL|ONSHIPCAST|TESTCANSEE)
 */
static int sp_analysesong_unit(castorder * co)
{
    unit *u;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    spellparameter *pa = co->par;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
     * abbrechen aber kosten lassen */
    if (pa->param[0]->flag == TARGET_RESISTS)
        return cast_level;

    u = pa->param[0]->data.u;

    magicanalyse_unit(u, mage, force);

    return cast_level;
}

static bool can_charm(const unit * u, int maxlevel)
{
    const skill_t expskills[] =
    { SK_ALCHEMY, SK_HERBALISM, SK_MAGIC, SK_SPY, SK_TACTICS, NOSKILL };
    skill *sv = u->skills;

    if (fval(u, UFL_HERO))
        return false;

    for (; sv != u->skills + u->skill_size; ++sv) {
        int l = 0, h = 5;
        skill_t sk = sv->id;
        assert(expskills[h] == NOSKILL);
        while (l < h) {
            int m = (l + h) / 2;
            if (sk == expskills[m]) {
                if (skill_limit(u->faction, sk) != INT_MAX) {
                    return false;
                }
                else if ((int)sv->level > maxlevel) {
                    return false;
                }
                break;
            }
            else if (sk > expskills[m])
                l = m + 1;
            else
                h = m;
        }
    }
    return true;
}

/* ------------------------------------------------------------- */
/* Name:     Charming
 * Stufe:   13
 * Gebiet:   Cerddor
 * Flag:     UNITSPELL
 * Wirkung:
 *   bezauberte Einheit wechselt 'virtuell' die Partei und fuehrt fremde
 *   Befehle aus.
 *   Dauer: 3 - force+2 Wochen
 *   Wirkt gegen Magieresistenz
 *
 *   wirkt auf eine Einheit mit maximal Talent Personen normal. Fuer jede
 *   zusaetzliche Person gibt es einen Bonus auf Magieresistenz, also auf
 *   nichtgelingen, von 10%.
 *
 *   Das hoechste Talent der Einheit darf maximal so hoch sein wie das
 *   Magietalent des Magiers. Fuer jeden Talentpunkt mehr gibt es einen
 *   Bonus auf Magieresistenz von 15%, was dazu fuehrt, das bei +2 Stufen
 *   die Magiersistenz bei 90% liegt.
 *
 *   Migrantenzaehlung muss Einheit ueberspringen
 *
 *   Attackiere verbieten
 * Flags:
 *   (UNITSPELL | TESTCANSEE)
 */
static int sp_charmingsong(castorder * co)
{
    unit *target;
    int duration;
    skill_t i;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    spellparameter *pa = co->par;
    int resist_bonus = 0;
    int tb = 0;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    target = pa->param[0]->data.u;

    /* Auf eigene Einheiten versucht zu zaubern? Garantiert Tippfehler */
    if (target->faction == mage->faction) {
        /* Die Einheit ist eine der unsrigen */
        cmistake(mage, co->order, 45, MSG_MAGIC);
    }
    /* niemand mit teurem Talent */
    if (!can_charm(target, cast_level / 2)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_noexpensives", "target", target));
        return 0;
    }

    /* Magieresistensbonus fuer mehr als Stufe Personen */
    if (target->number > force) {
        resist_bonus += (int)((target->number - force) * 10);
    }
    /* Magieresistensbonus fuer hoehere Talentwerte */
    for (i = 0; i < MAXSKILLS; i++) {
        int sk = effskill(target, i, 0);
        if (tb < sk)
            tb = sk;
    }
    tb -= effskill(mage, SK_MAGIC, 0);
    if (tb > 0) {
        resist_bonus += tb * 15;
    }
    /* Magieresistenz */
    if (target_resists_magic(mage, target, TYP_UNIT, resist_bonus)) {
        report_failure(mage, co->order);
        return 0;
    }

    duration = 3 + rng_int() % (int)force;
    {
        trigger *trestore = trigger_changefaction(target, target->faction);
        /* laeuft die Dauer ab, setze Partei zurueck */
        add_trigger(&target->attribs, "timer", trigger_timeout(duration, trestore));
        /* wird die alte Partei von Target aufgeloest, dann auch diese Einheit */
        add_trigger(&target->faction->attribs, "destroy", trigger_killunit(target));
        /* wird die neue Partei von Target aufgeloest, dann auch diese Einheit */
        add_trigger(&mage->faction->attribs, "destroy", trigger_killunit(target));
    }
    /* sperre ATTACKIERE, GIB PERSON und ueberspringe Migranten */
    create_curse(mage, &target->attribs, &ct_slavery, force, duration, zero_effect, 0);

    /* setze Partei um und loesche langen Befehl aus Sicherheitsgruenden */
    u_setfaction(target, mage->faction);
    set_order(&target->thisorder, NULL);

    /* setze Parteitarnung, damit nicht sofort klar ist, wer dahinter
     * steckt */
    if (rule_stealth_anon()) {
        fset(target, UFL_ANON_FACTION);
    }

    ADDMSG(&mage->faction->msgs, msg_message("charming_effect",
        "mage unit duration", mage, target, duration));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Gesang des wachen Geistes
 * Stufe:   10
 * Gebiet:   Cerddor
 * Kosten:   SPC_LEVEL
 * Wirkung:
 *  Bringt einmaligen Bonus von +15% auf Magieresistenz.  Wirkt auf alle
 *  Aliierten (HELFE BEWACHE) in der Region.
 *  Dauert Stufe Wochen an, ist nicht kumulativ.
 * Flag:
 *   (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
static int sp_song_resistmagic(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = (int)force + 1;

    create_curse(mage, &r->attribs, &ct_goodmagicresistancezone,
        force, duration, 15, 0);

    /* Erfolg melden */
    ADDMSG(&mage->faction->msgs, msg_message("regionmagic_effect",
        "unit region command", mage, mage->region, co->order));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Gesang des schwachen Geistes
 * Stufe:   12
 * Gebiet:   Cerddor
 * Wirkung:
 *  Bringt einmaligen Malus von -15% auf Magieresistenz.
 *  Wirkt auf alle Nicht-Aliierten (HELFE BEWACHE) in der Region.
 *  Dauert Stufe Wochen an, ist nicht kumulativ.
 * Flag:
 *   (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
static int sp_song_susceptmagic(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = (int)force + 1;

    create_curse(mage, &r->attribs, &ct_badmagicresistancezone,
        force, duration, 15, 0);

    ADDMSG(&mage->faction->msgs, msg_message("regionmagic_effect",
        "unit region command", mage, mage->region, co->order));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Aufruhr beschwichtigen
 * Stufe:   15
 * Gebiet:   Cerddor
 * Flag:     FARCASTING
 * Wirkung:
 *   zerstreut einen Monsterbauernmob, Antimagie zu 'Aufruhr
 *   verursachen'
 */

static int sp_rallypeasantmob(castorder * co)
{
    unit *u, *un;
    int erfolg = 0;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    message *msg;
    curse *c;

    for (u = r->units; u; u = un) {
        un = u->next;
        if (is_monsters(u->faction) && u_race(u) == get_race(RC_PEASANT)) {
            rsetpeasants(r, rpeasants(r) + u->number);
            rsetmoney(r, rmoney(r) + get_money(u));
            set_money(u, 0);
            setguard(u, false);
            set_number(u, 0);
            erfolg = cast_level;
        }
    }

    c = get_curse(r->attribs, &ct_riotzone);
    if (c != NULL) {
        remove_curse(&r->attribs, c);
    }

    msg = msg_message("cast_rally_effect", "mage region", mage, r);
    r_addmessage(r, NULL, msg);
    msg_release(msg);
    return erfolg;
}

/* ------------------------------------------------------------- */
/* Name:     Aufruhr verursachen
 * Stufe:   16
 * Gebiet:   Cerddor
 * Wirkung:
 *  Wiegelt 60% bis 90% der Bauern einer Region auf.  Bauern werden ein
 *  gro�er Mob, der zur Monsterpartei gehoert und die Region bewacht.
 *  Regionssilber sollte auch nicht durch Unterhaltung gewonnen werden
 *  koennen.
 *
 *   Fehlt: Triggeraktion: loeste Bauernmob auf und gib alles an Region,
 *   dann koennen die Bauernmobs ihr Silber mitnehmen und bleiben x
 *   Wochen bestehen
 *
 *  alternativ: Loesen sich langsam wieder auf
 * Flag:
 *   (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
static int sp_raisepeasantmob(castorder * co)
{
    unit *u;
    attrib *a;
    int n;
    int anteil;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = (int)force + 1;
    faction *monsters = get_monsters();
    message *msg;

    anteil = 6 + (rng_int() % 4);

    n = rpeasants(r) * anteil / 10;
    n = MAX(0, n);
    n = MIN(n, rpeasants(r));

    if (n <= 0) {
        report_failure(mage, co->order);
        return 0;
    }

    rsetpeasants(r, rpeasants(r) - n);
    assert(rpeasants(r) >= 0);

    u =
        create_unit(r, monsters, n, get_race(RC_PEASANT), 0, LOC(monsters->locale,
            "furious_mob"), NULL);
    fset(u, UFL_ISNEW);
    setguard(u, true);
    a = a_new(&at_unitdissolve);
    a->data.ca[0] = 1;            /* An rpeasants(r). */
    a->data.ca[1] = 15;           /* 15% */
    a_add(&u->attribs, a);

    create_curse(mage, &r->attribs, &ct_riotzone, (float)cast_level, duration,
        (float)anteil, 0);

    msg = msg_message("sp_raisepeasantmob_effect", "mage region", mage, r);
    report_spell(mage, r, msg);
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Ritual der Aufnahme / Migrantenwerben
 * Stufe:   9
 * Gebiet:   Cerddor
 * Wirkung:
 *  Bis zu Stufe Personen fremder Rasse koennen angeworben werden. Die
 *  angeworbene Einheit muss kontaktieren. Keine teuren Talente
 *
 * Flag:
 *  (UNITSPELL | SPELLLEVEL | TESTCANSEE)
 */
static int sp_migranten(castorder * co)
{
    unit *target;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    target = pa->param[0]->data.u;        /* Zieleinheit */

    /* Personen unserer Rasse koennen problemlos normal uebergeben werden */
    if (u_race(target) == mage->faction->race) {
        /* u ist von unserer Art, das Ritual waere verschwendete Aura. */
        ADDMSG(&mage->faction->msgs, msg_message("sp_migranten_fail1",
            "unit region command target", mage, mage->region, co->order, target));
    }
    /* Auf eigene Einheiten versucht zu zaubern? Garantiert Tippfehler */
    if (target->faction == mage->faction) {
        cmistake(mage, co->order, 45, MSG_MAGIC);
    }

    /* Keine Monstereinheiten */
    if (!playerrace(u_race(target))) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_nomonsters", ""));
        return 0;
    }
    /* niemand mit teurem Talent */
    if (has_limited_skills(target)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_noexpensives", "target", target));
        return 0;
    }
    /* maximal Stufe Personen */
    if (target->number > cast_level || target->number > max_spellpoints(r, mage)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_toomanytargets", ""));
        return 0;
    }

    /* Kontakt pruefen (aus alter Teleportroutine uebernommen) */
    if (!ucontact(target, mage)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail::contact", "target", target));
        return 0;
    }
    u_setfaction(target, mage->faction);
    set_order(&target->thisorder, NULL);

    /* Erfolg melden */
    ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "sp_migranten",
        "target", target));

    return target->number;
}

/* ------------------------------------------------------------- */
/* Name:     Gesang der Friedfertigkeit
 * Stufe:   12
 * Gebiet:   Cerddor
 * Wirkung:
 *   verhindert jede Attacke fuer lovar(Stufe/2) Runden
 */

static int sp_song_of_peace(castorder * co)
{
    unit *u;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = 2 + lovar(force / 2);
    message *msg[2] = { NULL, NULL };

    create_curse(mage, &r->attribs, &ct_peacezone, force, duration,
        zero_effect, 0);

    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next) {
        if (!fval(u->faction, FFL_SELECT)) {
            message *m = NULL;
            fset(u->faction, FFL_SELECT);
            if (cansee(u->faction, r, mage, 0)) {
                if (msg[0] == NULL)
                    msg[0] = msg_message("song_of_peace_effect_0", "mage", mage);
                m = msg[0];
            }
            else {
                if (msg[1] == NULL)
                    msg[1] = msg_message("song_of_peace_effect_1", "");
                m = msg[1];
            }
            r_addmessage(r, u->faction, m);
        }
    }
    if (msg[0])
        msg_release(msg[0]);
    if (msg[1])
        msg_release(msg[1]);
    return cast_level;

}

/* ------------------------------------------------------------- */
/* Name:     Hohes Lied der Gaukelei
 * Stufe:   2
 * Gebiet:   Cerddor
 * Wirkung:
 *   Das Unterhaltungsmaximum steigt von 20% auf 40% des
 *   Regionsvermoegens. Der Spruch haelt Stufe Wochen an
 */

static int sp_generous(castorder * co)
{
    unit *u;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = (int)force + 1;
    double effect;
    message *msg[2] = { NULL, NULL };

    if (is_cursed(r->attribs, &ct_depression)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_generous", ""));
        return 0;
    }

    effect = 2;
    create_curse(mage, &r->attribs, &ct_generous, force, duration, effect,
        0);

    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next) {
        if (!fval(u->faction, FFL_SELECT)) {
            message *m = NULL;
            fset(u->faction, FFL_SELECT);
            if (cansee(u->faction, r, mage, 0)) {
                if (msg[0] == NULL)
                    msg[0] = msg_message("generous_effect_0", "mage", mage);
                m = msg[0];
            }
            else {
                if (msg[1] == NULL)
                    msg[1] = msg_message("generous_effect_1", "");
                m = msg[1];
            }
            r_addmessage(r, u->faction, m);
        }
    }
    if (msg[0])
        msg_release(msg[0]);
    if (msg[1])
        msg_release(msg[1]);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Anwerbung
 * Stufe:   4
 * Gebiet:   Cerddor
 * Wirkung:
 *   Bauern schliessen sich der eigenen Partei an
 *   ist zusaetzlich zur Rekrutierungsmenge in der Region
 * */

static int sp_recruit(castorder * co)
{
    unit *u;
    region *r = co_get_region(co);
    int num, maxp = rpeasants(r);
    double n;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    faction *f = mage->faction;
    const struct race *rc = f->race;

    if (maxp == 0) {
        report_failure(mage, co->order);
        return 0;
    }
    /* Immer noch zuviel auf niedrigen Stufen. Deshalb die Rekrutierungskosten
     * mit einfliessen lassen und dafuer den Exponenten etwas groe�er.
     * Wenn die Rekrutierungskosten deutlich hoeher sind als der Faktor,
     * ist das Verhaeltniss von ausgegebene Aura pro Bauer bei Stufe 2
     * ein mehrfaches von Stufe 1, denn in beiden Faellen gibt es nur 1
     * Bauer, nur die Kosten steigen. */
    n = (pow(force, 1.6) * 100) / f->race->recruitcost;
    if (rc->recruit_multi > 0) {
        double multp = (double)maxp / rc->recruit_multi;
        n = MIN(multp, n);
        n = MAX(n, 1);
        rsetpeasants(r, maxp - (int)(n * rc->recruit_multi));
    }
    else {
        n = MIN(maxp, n);
        n = MAX(n, 1);
        rsetpeasants(r, maxp - (int)n);
    }

    num = (int)n;
    u =
        create_unit(r, f, num, f->race, 0, LOC(f->locale,
        (num == 1 ? "peasant" : "peasant_p")), mage);
    set_order(&u->thisorder, default_order(f->locale));

    ADDMSG(&mage->faction->msgs, msg_message("recruit_effect", "mage amount",
        mage, num));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:    Wanderprediger - Gro�e Anwerbung
 * Stufe:   14
 * Gebiet:  Cerddor
 * Wirkung:
 *   Bauern schliessen sich der eigenen Partei an
 *   ist zusaetzlich zur Rekrutierungsmenge in der Region
 * */

static int sp_bigrecruit(castorder * co)
{
    unit *u;
    region *r = co_get_region(co);
    int n, maxp = rpeasants(r);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    faction *f = mage->faction;
    message *msg;

    if (maxp <= 0) {
        report_failure(mage, co->order);
        return 0;
    }
    /* Fuer vergleichbare Erfolge bei unterschiedlichen Rassen die
     * Rekrutierungskosten mit einfliessen lassen. */

    n = (int)force + lovar((force * force * 1000) / (float)f->race->recruitcost);
    if (f->race == get_race(RC_ORC)) {
        n = MIN(2 * maxp, n);
        n = MAX(n, 1);
        rsetpeasants(r, maxp - (n + 1) / 2);
    }
    else {
        n = MIN(maxp, n);
        n = MAX(n, 1);
        rsetpeasants(r, maxp - n);
    }

    u =
        create_unit(r, f, n, f->race, 0, LOC(f->locale,
        (n == 1 ? "peasant" : "peasant_p")), mage);
    set_order(&u->thisorder, default_order(f->locale));

    msg = msg_message("recruit_effect", "mage amount", mage, n);
    r_addmessage(r, mage->faction, msg);
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Aushorchen
 * Stufe:   7
 * Gebiet:   Cerddor
 * Wirkung:
 *  Erliegt die Einheit dem Zauber, so wird sie dem Magier alles
 *  erzaehlen, was sie ueber die gefragte Region wei�. Ist in der Region
 *  niemand ihrer Partei, so wei� sie nichts zu berichten.  Auch kann
 *  sie nur das erzaehlen, was sie selber sehen koennte.
 * Flags:
 *   (UNITSPELL | TESTCANSEE)
 */

 /* restistenz der einheit pruefen */
static int sp_pump(castorder * co)
{
    unit *u, *target;
    region *rt;
    bool see = false;
    unit *mage = co->magician.u;
    spellparameter *pa = co->par;
    int cast_level = co->level;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    target = pa->param[0]->data.u;        /* Zieleinheit */

    if (fval(u_race(target), RCF_UNDEAD)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "error_not_on_undead", ""));
        return 0;
    }
    if (is_magic_resistant(mage, target, 0) || is_monsters(target->faction)) {
        report_failure(mage, co->order);
        return 0;
    }

    rt = pa->param[1]->data.r;

    for (u = rt->units; u; u = u->next) {
        if (u->faction == target->faction)
            see = true;
    }

    if (see) {
        ADDMSG(&mage->faction->msgs, msg_message("pump_effect", "mage unit tregion",
            mage, target, rt));
    }
    else {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order, "spellfail_pump",
            "target tregion", target, rt));
        return cast_level / 2;
    }

    set_observer(rt, mage->faction, effskill(target, SK_PERCEPTION, 0), 2);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Verfuehrung
 * Stufe:   6
 * Gebiet:   Cerddor
 * Wirkung:
 *  Betoert eine Einheit, so das sie ihm den groe�ten Teil ihres Bargelds
 *  und 50% ihres Besitzes schenkt. Sie behaelt jedoch immer soviel, wie
 *  sie zum ueberleben braucht. Wirkt gegen Magieresistenz.
 *  MIN(Stufe*1000$, u->money - maintenace)
 *  Von jedem Item wird 50% abgerundet ermittelt und uebergeben. Dazu
 *  kommt Itemzahl%2 mit 50% chance
 *
 * Flags:
 * (UNITSPELL | TESTRESISTANCE | TESTCANSEE)
 */
static int sp_seduce(castorder * co)
{
    const resource_type *rsilver = get_resourcetype(R_SILVER);
    unit *target;
    item **itmp, *items = 0;
    unit *mage = co->magician.u;
    spellparameter *pa = co->par;
    int cast_level = co->level;
    double force = co->force;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    target = pa->param[0]->data.u;        /* Zieleinheit */

    if (fval(u_race(target), RCF_UNDEAD)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_noundead", ""));
        return 0;
    }

    /* Erfolgsmeldung */

    itmp = &target->items;
    while (*itmp) {
        item *itm = *itmp;
        int loot;
        if (itm->type->rtype == rsilver) {
            loot =
                MIN(cast_level * 1000, get_money(target) - (maintenance_cost(target)));
            loot = MAX(loot, 0);
        }
        else {
            loot = itm->number / 2;
            if (itm->number % 2) {
                loot += rng_int() % 2;
            }
            if (loot > 0) {
                loot = MIN(loot, (int)(force * 5));
            }
        }
        if (loot > 0) {
            i_change(&mage->items, itm->type, loot);
            i_change(&items, itm->type, loot);
            i_change(itmp, itm->type, -loot);
        }
        if (*itmp == itm)
            itmp = &itm->next;
    }

    if (items) {
        ADDMSG(&mage->faction->msgs, msg_message("seduce_effect_0", "mage unit items",
            mage, target, items));
        i_freeall(&items);
        ADDMSG(&target->faction->msgs, msg_message("seduce_effect_1", "unit",
            target));
    }
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Monster friedlich stimmen
 * Stufe:   6
 * Gebiet:   Cerddor
 * Wirkung:
 *  verhindert Angriffe des bezauberten Monsters auf die Partei des
 *  Barden fuer Stufe Wochen. Nicht uebertragbar, dh Verbuendete werden vom
 *  Monster natuerlich noch angegriffen. Wirkt nicht gegen Untote
 *  Jede Einheit kann maximal unter einem Beherrschungszauber dieser Art
 *  stehen, dh wird auf die selbe Einheit dieser Zauber von einem
 *  anderen Magier nochmal gezaubert, schlaegt der Zauber fehl.
 *
 * Flags:
 * (UNITSPELL | ONSHIPCAST | TESTRESISTANCE | TESTCANSEE)
 */

static int sp_calm_monster(castorder * co)
{
    curse *c;
    unit *target;
    unit *mage = co->magician.u;
    spellparameter *pa = co->par;
    int cast_level = co->level;
    double force = co->force;
    double effect;
    message *msg;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    target = pa->param[0]->data.u;        /* Zieleinheit */

    if (fval(u_race(target), RCF_UNDEAD)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_noundead", ""));
        return 0;
    }

    effect = mage->faction->subscription;
    c = create_curse(mage, &target->attribs, &ct_calmmonster, force,
        (int)force, effect, 0);
    if (c == NULL) {
        report_failure(mage, co->order);
        return 0;
    }

    msg = msg_message("calm_effect", "mage unit", mage, target);
    r_addmessage(mage->region, mage->faction, msg);
    msg_release(msg);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     schaler Wein
 * Stufe:   7
 * Gebiet:   Cerddor
 * Wirkung:
 *  wird gegen Magieresistenz gezaubert Das Opfer vergisst bis zu
 *  Talenttage seines hoechsten Talentes und tut die Woche nix.
 *  Nachfolgende Zauber sind erschwert.
 *  Wirkt auf bis zu 10 Personen in der Einheit
 *
 * Flags:
 * (UNITSPELL | TESTRESISTANCE | TESTCANSEE)
 */

static int sp_headache(castorder * co)
{
    skill *smax = NULL;
    int i;
    unit *target;
    unit *mage = co->magician.u;
    spellparameter *pa = co->par;
    int cast_level = co->level;
    message *msg;

    /* Macht alle nachfolgenden Zauber doppelt so teuer */
    countspells(mage, 1);

    target = pa->param[0]->data.u;        /* Zieleinheit */

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (target->number == 0 || pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* finde das groe�te Talent: */
    for (i = 0; i != target->skill_size; ++i) {
        skill *sv = target->skills + i;
        if (smax == NULL || skill_compare(sv, smax) > 0) {
            smax = sv;
        }
    }
    if (smax != NULL) {
        /* wirkt auf maximal 10 Personen */
        unsigned int change = MIN(10, target->number) * (rng_uint() % 2 + 1) / target->number;
        reduce_skill(target, smax, change);
    }
    set_order(&target->thisorder, NULL);

    msg = msg_message("hangover_effect_0", "mage unit", mage, target);
    r_addmessage(mage->region, mage->faction, msg);
    msg_release(msg);

    msg = msg_message("hangover_effect_1", "unit", target);
    r_addmessage(target->region, target->faction, msg);
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Mob
 * Stufe:   10
 * Gebiet:   Cerddor
 * Wirkung:
 *   Wiegelt Stufe*250 Bauern zu einem Mob auf, der sich der Partei des
 *   Magier anschliesst Pro Woche beruhigen sich etwa 15% wieder und
 *   kehren auf ihre Felder zurueck
 *
 * Flags:
 * (SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
static int sp_raisepeasants(castorder * co)
{
    int bauern;
    unit *u2;
    attrib *a;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    message *msg;

    if (rpeasants(r) == 0) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "error_nopeasants", ""));
        return 0;
    }
    bauern = MIN(rpeasants(r), (int)(power * 250));
    rsetpeasants(r, rpeasants(r) - bauern);

    u2 =
        create_unit(r, mage->faction, bauern, get_race(RC_PEASANT), 0,
            LOC(mage->faction->locale, "furious_mob"), mage);

    fset(u2, UFL_LOCKED);
    if (rule_stealth_anon()) {
        fset(u2, UFL_ANON_FACTION);
    }

    a = a_new(&at_unitdissolve);
    a->data.ca[0] = 1;            /* An rpeasants(r). */
    a->data.ca[1] = 15;           /* 15% */
    a_add(&u2->attribs, a);

    msg =
        msg_message("sp_raisepeasants_effect", "mage region amount", mage, r,
            u2->number);
    r_addmessage(r, NULL, msg);
    if (mage->region != r) {
        add_message(&mage->faction->msgs, msg);
    }
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Truebsal
 * Stufe:      11
 * Kategorie:  Region, negativ
 * Wirkung:
 *  in der Region kann fuer einige Wochen durch Unterhaltung kein Geld
 *  mehr verdient werden
 *
 * Flag:
 * (FARCASTING | REGIONSPELL | TESTRESISTANCE)
 */
static int sp_depression(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = (int)force + 1;
    message *msg;

    create_curse(mage, &r->attribs, &ct_depression, force, duration,
        zero_effect, 0);

    msg = msg_message("sp_depression_effect", "mage region", mage, r);
    r_addmessage(r, NULL, msg);
    if (mage->region != r) {
        add_message(&mage->faction->msgs, msg);
    }
    msg_release(msg);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* TRAUM - Illaun */
/* ------------------------------------------------------------- */

/* Name:       Seelenfrieden
 * Stufe:      2
 * Kategorie:  Region, positiv
 * Gebiet:     Illaun
 * Wirkung:
 *   Reduziert Untotencounter
 * Flag: (0)
 */

int sp_puttorest(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int dead = deathcount(r);
    int laid_to_rest = dice((int)(co->force * 2), 100);
    message *seen = msg_message("puttorest", "mage", mage);
    message *unseen = msg_message("puttorest", "mage", NULL);

    laid_to_rest = MAX(laid_to_rest, dead);

    deathcounts(r, -laid_to_rest);

    report_effect(r, mage, seen, unseen);
    msg_release(seen);
    msg_release(unseen);
    return co->level;
}

/* Name:       Traumschloe�chen
 * Stufe:      3
 * Kategorie:  Region, Gebaeude, positiv
 * Gebiet:     Illaun
 * Wirkung:
 *  Mit Hilfe dieses Zaubers kann der Traumweber die Illusion eines
 *  beliebigen Gebaeudes erzeugen. Die Illusion kann betreten werden, ist
 *  aber ansonsten funktionslos und benoetigt auch keinen Unterhalt
 * Flag: (0)
 */

int sp_icastle(castorder * co)
{
    building *b;
    const building_type *type;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;
    const char *bname;
    message *msg;
    const building_type *bt_illusion = bt_find("illusioncastle");

    if (!bt_illusion) {
        return 0;
    }

    if ((type =
        findbuildingtype(pa->param[0]->data.xs, mage->faction->locale)) == NULL) {
        type = bt_find("castle");
    }

    b = new_building(bt_illusion, r, mage->faction->locale);

    /* Groe�e festlegen. */
    if (type == bt_illusion) {
        b->size = (rng_int() % (int)((power * power) + 1) * 10);
    }
    else if (type->maxsize > 0) {
        b->size = type->maxsize;
    }
    else {
        b->size = ((rng_int() % (int)(power)) + 1) * 5;
    }

    bname = LOC(mage->faction->locale, buildingtype(type, b, 0));
    building_setname(b, bname);

    /* TODO: Auf timeout und action_destroy umstellen */
    make_icastle(b, type, 2 + (rng_int() % (int)(power)+1) * (rng_int() % (int)(power)+1));

    if (mage->region == r) {
        if (leave(mage, false)) {
            u_set_building(mage, b);
        }
    }

    ADDMSG(&mage->faction->msgs, msg_message("icastle_create",
        "unit region command", mage, mage->region, co->order));

    msg = msg_message("sp_icastle_effect", "region", r);
    report_spell(mage, r, msg);
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:    Gestaltwandlung
 * Stufe:   3
 * Gebiet:  Illaun
 * Wirkung:
 *  Zieleinheit erscheint fuer (Stufe) Wochen als eine andere Gestalt
 *  (wie bei daemonischer Rassetarnung).
 * Syntax: ZAUBERE "Gestaltwandlung"  <einheit>     <rasse>
 * Flags:
 *  (UNITSPELL | SPELLLEVEL)
 */

int sp_illusionary_shapeshift(castorder * co)
{
    unit *u;
    const race *rc;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;
    const race *irace;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
     * abbrechen aber kosten lassen */
    if (pa->param[0]->flag == TARGET_RESISTS)
        return cast_level;

    u = pa->param[0]->data.u;

    rc = findrace(pa->param[1]->data.xs, mage->faction->locale);
    if (rc == NULL) {
        cmistake(mage, co->order, 202, MSG_MAGIC);
        return 0;
    }

    /* aehnlich wie in laws.c:setealth() */
    if (!playerrace(rc)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "sp_shapeshift_fail", "target race", u, rc));
        return 0;
    }
    irace = u_irace(u);
    if (irace == u_race(u)) {
        trigger *trestore = trigger_changerace(u, NULL, irace);
        add_trigger(&u->attribs, "timer", trigger_timeout((int)power + 3,
            trestore));
        u->irace = rc;
    }

    ADDMSG(&mage->faction->msgs, msg_message("shapeshift_effect",
        "mage target race", mage, u, rc));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Regionstraum analysieren
 * Stufe:   9
 * Aura:     18
 * Kosten:   SPC_FIX
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstaerke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 */
int sp_analyseregionsdream(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;

    magicanalyse_region(r, mage, cast_level);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Traumbilder erkennen
 * Stufe:   5
 * Aura:     12
 * Kosten:   SPC_FIX
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstaerke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 */
int sp_analysedream(castorder * co)
{
    unit *u;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
     * abbrechen aber kosten lassen */
    if (pa->param[0]->flag == TARGET_RESISTS)
        return cast_level;

    u = pa->param[0]->data.u;
    magicanalyse_unit(u, mage, cast_level);

    return cast_level;
}

static int sp_gbdreams(castorder * co, int effect)
{
    int duration;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    region *r = co_get_region(co);

    /* wirkt erst in der Folgerunde, soll mindestens eine Runde wirken,
    * also duration+2 */
    duration = (int)MAX(1, power / 2);    /* Stufe 1 macht sonst mist */
    duration = 2 + rng_int() % duration;

    /* Nichts machen als ein entsprechendes Attribut in die Region legen. */
    create_curse(mage, &r->attribs, &ct_gbdream, power, duration, effect, 0);

    /* Erfolg melden */
    ADDMSG(&mage->faction->msgs, msg_message("regionmagic_effect",
        "unit region command", mage, mage->region, co->order));

    return cast_level;
}


/* ------------------------------------------------------------- */
/* Name:       Schlechte Traeume
 * Stufe:      10
 * Kategorie:  Region, negativ
 * Wirkung:
 *   Dieser Zauber ermoeglicht es dem Traeumer, den Schlaf aller
 *   nichtaliierten Einheiten (HELFE BEWACHE) in der Region so starkzu
 *   stoeren, das sie 1 Talentstufe in allen Talenten
 *   voruebergehend verlieren. Der Zauber wirkt erst im Folgemonat.
 *
 * Flags:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 * */
int sp_baddreams(castorder * co)
{
    return sp_gbdreams(co, -1);
}

/* ------------------------------------------------------------- */
/* Name:       Schoene Traeume
 * Stufe:      8
 * Kategorie:
 * Wirkung:
 *   Dieser Zauber ermoeglicht es dem Traeumer, den Schlaf aller aliierten
 *   Einheiten in der Region so zu beeinflussen, da� sie fuer einige Zeit
 *   einen Bonus von 1 Talentstufe in allen Talenten
 *   bekommen. Der Zauber wirkt erst im Folgemonat.
 * Flags:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 */
int sp_gooddreams(castorder * co)
{
    return sp_gbdreams(co, 1);
}

/* ------------------------------------------------------------- */
/* Name: Seelenkopie / Doppelganger
 * Stufe:      9
 * Kategorie:
 * Wirkung:
 *   Es wird eine Kloneinheit erzeugt, die nichts kann. Stirbt der
 *   Magier, wird er mit einer Wahrscheinlichkeit von 90% in den Klon
 *   transferiert.
 * Flags:
 * (NOTFAMILARCAST)
 */
int sp_clonecopy(castorder * co)
{
    unit *clone;
    region *r = co_get_region(co);
    region *target_region = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    message *msg;
    char name[NAMESIZE];

    if (get_clone(mage) != NULL) {
        cmistake(mage, co->order, 298, MSG_MAGIC);
        return 0;
    }

    slprintf(name, sizeof(name), (const char *)LOC(mage->faction->locale,
        "clone_of"), unitname(mage));
    clone =
        create_unit(target_region, mage->faction, 1, get_race(RC_CLONE), 0, name,
            mage);
    setstatus(clone, ST_FLEE);
    fset(clone, UFL_LOCKED);

    create_newclone(mage, clone);

    msg = msg_message("sp_clone_effect", "mage", mage);
    r_addmessage(r, mage->faction, msg);
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
int sp_dreamreading(castorder * co)
{
    unit *u;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;
    double power = co->force;
    message *msg;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
     * abbrechen aber kosten lassen */
    if (pa->param[0]->flag == TARGET_RESISTS)
        return cast_level;

    u = pa->param[0]->data.u;

    /* Illusionen und Untote abfangen. */
    if (fval(u_race(u), RCF_UNDEAD | RCF_ILLUSIONARY)) {
        ADDMSG(&mage->faction->msgs, msg_unitnotfound(mage, co->order,
            pa->param[0]));
        return 0;
    }

    /* Entfernung */
    if (distance(mage->region, u->region) > power) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_distance", ""));
        return 0;
    }

    set_observer(u->region, mage->faction, effskill(u, SK_PERCEPTION, u->region), 2);

    msg =
        msg_message("sp_dreamreading_effect", "mage unit region", mage, u,
            u->region);
    r_addmessage(r, mage->faction, msg);
    msg_release(msg);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Wirkt power/2 Runden auf bis zu power^2 Personen
 * mit einer Chance von 5% vermehren sie sich  */
int sp_sweetdreams(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;
    int men, n;
    int duration = (int)(power / 2) + 1;
    int opfer = (int)(power * power);

    /* Schleife ueber alle angegebenen Einheiten */
    for (n = 0; n < pa->length; n++) {
        unit *u;
        double effect;
        message *msg;
        /* sollte nie negativ werden */
        if (opfer < 1)
            break;

        if (pa->param[n]->flag == TARGET_RESISTS ||
            pa->param[n]->flag == TARGET_NOTFOUND)
            continue;

        /* Zieleinheit */
        u = pa->param[n]->data.u;

        if (!ucontact(u, mage)) {
            cmistake(mage, co->order, 40, MSG_EVENT);
            continue;
        }
        men = MIN(opfer, u->number);
        opfer -= men;

        /* Nichts machen als ein entsprechendes Attribut an die Einheit legen. */
        effect = 0.05f;
        create_curse(mage, &u->attribs, &ct_orcish, power, duration, effect, men);

        msg = msg_message("sp_sweetdreams_effect", "mage unit region", mage, u, r);
        r_addmessage(r, mage->faction, msg);
        if (u->faction != mage->faction) {
            r_addmessage(r, u->faction, msg);
        }
        msg_release(msg);
    }
    return cast_level;
}

/* ------------------------------------------------------------- */
int sp_disturbingdreams(castorder * co)
{
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    int duration = 1 + (int)(power / 6);
    double effect;

    effect = 10;
    create_curse(mage, &r->attribs, &ct_badlearn, power, duration, effect, 0);

    ADDMSG(&mage->faction->msgs, msg_message("sp_disturbingdreams_effect",
        "mage region", mage, r));
    return cast_level;
}

/* ------------------------------------------------------------- */
/* ASTRAL / THEORIE / M_TYBIED */
/* ------------------------------------------------------------- */
/* Name:     Magie analysieren
 * Stufe:   1
 * Aura:     1
 * Kosten:   SPC_LINEAR
 * Komponenten:
 *
 * Wirkung:
 *  Zeigt die Verzauberungen eines Objekts an (curse->name,
 *  curse::info). Aus der Differenz Spruchstaerke und Curse->vigour
 *  ergibt sich die Chance den Spruch zu identifizieren ((force -
 *  c->vigour)*10 + 100 %).
 *
 * Flags:
 *  UNITSPELL, SHIPSPELL, BUILDINGSPELL
 */

int sp_analysemagic(castorder * co)
{
    int obj;
    unit *mage = co_get_caster(co);
    int cast_level = co->level;
    spellparameter *pa = co->par;

    if (!pa->param) {
        return 0;
    }
    /* Objekt ermitteln */
    obj = pa->param[0]->typ;

    switch (obj) {
    case SPP_REGION:
    {
        region *tr = pa->param[0]->data.r;
        magicanalyse_region(tr, mage, cast_level);
        break;
    }
    case SPP_TEMP:
    case SPP_UNIT:
    {
        unit *u;
        u = pa->param[0]->data.u;
        magicanalyse_unit(u, mage, cast_level);
        break;
    }
    case SPP_BUILDING:
    {
        building *b;
        b = pa->param[0]->data.b;
        magicanalyse_building(b, mage, cast_level);
        break;
    }
    case SPP_SHIP:
    {
        ship *sh;
        sh = pa->param[0]->data.sh;
        magicanalyse_ship(sh, mage, cast_level);
        break;
    }
    default:
        /* Fehlerhafter Parameter */
        return 0;
    }

    return cast_level;
}

/* ------------------------------------------------------------- */

int sp_itemcloak(castorder * co)
{
    unit *target;
    unit *mage = co->magician.u;
    spellparameter *pa = co->par;
    int cast_level = co->level;
    double power = co->force;
    int duration = (int)MAX(2.0, power + 1);      /* works in the report, and ageing this round would kill it if it's <=1 */

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* Zieleinheit */
    target = pa->param[0]->data.u;

    create_curse(mage, &target->attribs, &ct_itemcloak, power, duration,
        zero_effect, 0);
    ADDMSG(&mage->faction->msgs, msg_message("itemcloak", "mage target", mage,
        target));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Magieresistenz erhoehen
 * Stufe:   3
 * Aura:     5 MP
 * Kosten:   SPC_LEVEL
 * Komponenten:
 *
 * Wirkung:
 *   erhoeht die Magierestistenz der Personen um 20 Punkte fuer 6 Wochen
 *   Wirkt auf Stufe*5 Personen kann auf mehrere Einheiten gezaubert
 *   werden, bis die Zahl der moeglichen Personen erschoepft ist
 *
 * Flags:
 *   UNITSPELL
 */
int sp_resist_magic_bonus(castorder * co)
{
    unit *u;
    int n, m;
    int duration = 6;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;
    /* Pro Stufe koennen bis zu 5 Personen verzaubert werden */
    double maxvictims = 5 * power;
    int victims = (int)maxvictims;

    /* Schleife ueber alle angegebenen Einheiten */
    for (n = 0; n < pa->length; n++) {
        message *msg;
        /* sollte nie negativ werden */
        if (victims < 1)
            break;

        if (pa->param[n]->flag == TARGET_RESISTS
            || pa->param[n]->flag == TARGET_NOTFOUND)
            continue;

        u = pa->param[n]->data.u;

        m = MIN(u->number, victims);
        victims -= m;

        create_curse(mage, &u->attribs, &ct_magicresistance,
            power, duration, 20, m);

        msg = msg_message("magicresistance_effect", "unit", u);
        add_message(&u->faction->msgs, msg);

        /* und noch einmal dem Magier melden */
        if (u->faction != mage->faction) {
            add_message(&mage->faction->msgs, msg);
        }
        msg_release(msg);
    }

    cast_level = MIN(cast_level, (int)(cast_level * (victims + 4) / maxvictims));
    return MAX(cast_level, 1);
}

/** spell 'Astraler Weg'.
 * Syntax "ZAUBERE [STUFE n] 'Astraler Weg' <Einheit-Nr> [<Einheit-Nr> ...]",
 *
 * Parameter:
 * pa->param[0]->data.xs
 */
int sp_enterastral(castorder * co)
{
    region *rt, *ro;
    unit *u, *u2;
    int remaining_cap;
    int n, w;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;

    switch (getplaneid(r)) {
    case 0:
        rt = r_standard_to_astral(r);
        ro = r;
        break;
    default:
        cmistake(mage, co->order, 190, MSG_MAGIC);
        return 0;
    }

    if (!rt || fval(rt->terrain, FORBIDDEN_REGION)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "feedback_no_astralregion", ""));
        return 0;
    }

    if (is_cursed(rt->attribs, &ct_astralblock)
        || is_cursed(ro->attribs, &ct_astralblock)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_astralblock", ""));
        return 0;
    }

    remaining_cap = (int)((power - 3) * 1500);

    /* fuer jede Einheit in der Kommandozeile */
    for (n = 0; n < pa->length; n++) {
        if (pa->param[n]->flag == TARGET_NOTFOUND)
            continue;
        u = pa->param[n]->data.u;

        if (!ucontact(u, mage)) {
            if (power > 10 && !is_magic_resistant(mage, u, 0) && can_survive(u, rt)) {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "feedback_no_contact_no_resist", "target", u));
                ADDMSG(&u->faction->msgs, msg_message("send_astral", "unit target",
                    mage, u));
            }
            else {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "feedback_no_contact_resist", "target", u));
                ADDMSG(&u->faction->msgs, msg_message("try_astral", "unit target", mage,
                    u));
                continue;
            }
        }

        w = weight(u);
        if (!can_survive(u, rt)) {
            cmistake(mage, co->order, 231, MSG_MAGIC);
        }
        else if (remaining_cap - w < 0) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "fail_tooheavy", "target", u));
        }
        else {
            message *m;
            remaining_cap = remaining_cap - w;
            move_unit(u, rt, NULL);

            /* Meldungen in der Ausgangsregion */

            for (u2 = r->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);
            m = NULL;
            for (u2 = r->units; u2; u2 = u2->next) {
                if (!fval(u2->faction, FFL_SELECT)) {
                    if (cansee(u2->faction, r, u, 0)) {
                        fset(u2->faction, FFL_SELECT);
                        if (!m)
                            m = msg_message("astral_disappear", "unit", u);
                        r_addmessage(r, u2->faction, m);
                    }
                }
            }
            if (m)
                msg_release(m);

            /* Meldungen in der Zielregion */

            for (u2 = rt->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);
            m = NULL;
            for (u2 = rt->units; u2; u2 = u2->next) {
                if (!fval(u2->faction, FFL_SELECT)) {
                    if (cansee(u2->faction, rt, u, 0)) {
                        fset(u2->faction, FFL_SELECT);
                        if (!m)
                            m = msg_message("astral_appear", "unit", u);
                        r_addmessage(rt, u2->faction, m);
                    }
                }
            }
            if (m)
                msg_release(m);
        }
    }
    return cast_level;
}

/** Spell 'Astraler Ruf' / 'Astral Call'.
 */
int sp_pullastral(castorder * co)
{
    region *rt, *ro;
    unit *u, *u2;
    region_list *rl, *rl2;
    int remaining_cap;
    int n, w;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;

    switch (getplaneid(r)) {
    case 1:
        rt = r;
        ro = pa->param[0]->data.r;
        rl = astralregions(r, NULL);
        rl2 = rl;
        while (rl2 != NULL) {
            region *r2 = rl2->data;
            if (r2->x == ro->x && r2->y == ro->y) {
                ro = r2;
                break;
            }
            rl2 = rl2->next;
        }
        if (!rl2) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "spellfail::nocontact", "target", rt));
            free_regionlist(rl);
            return 0;
        }
        free_regionlist(rl);
        break;
    default:
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_astralonly", ""));
        return 0;
    }

    if (is_cursed(rt->attribs, &ct_astralblock)
        || is_cursed(ro->attribs, &ct_astralblock)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_astralblock", ""));
        return 0;
    }

    remaining_cap = (int)((power - 3) * 1500);

    /* fuer jede Einheit in der Kommandozeile */
    for (n = 1; n < pa->length; n++) {
        spllprm *spobj = pa->param[n];
        if (spobj->flag == TARGET_NOTFOUND)
            continue;

        u = spobj->data.u;

        if (u->region != ro) {
            /* Report this as unit not found */
            if (spobj->typ == SPP_UNIT) {
                spobj->data.i = u->no;
            }
            else {
                spobj->data.i = ualias(u);
            }
            spobj->flag = TARGET_NOTFOUND;
            ADDMSG(&mage->faction->msgs, msg_unitnotfound(mage, co->order, spobj));
            return false;
        }

        if (!ucontact(u, mage)) {
            if (power > 12 && spobj->flag != TARGET_RESISTS && can_survive(u, rt)) {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "feedback_no_contact_no_resist", "target", u));
            }
            else {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "feedback_no_contact_resist", "target", u));
                ADDMSG(&u->faction->msgs, msg_message("try_astral", "unit target", mage,
                    u));
                continue;
            }
        }

        w = weight(u);

        if (!can_survive(u, rt)) {
            cmistake(mage, co->order, 231, MSG_MAGIC);
        }
        else if (remaining_cap - w < 0) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "fail_tooheavy", "target", u));
        }
        else {
            message *m;

            ADDMSG(&u->faction->msgs, msg_message("send_astral", "unit target", mage,
                u));
            remaining_cap = remaining_cap - w;
            move_unit(u, rt, NULL);

            /* Meldungen in der Ausgangsregion */

            for (u2 = r->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);
            m = NULL;
            for (u2 = r->units; u2; u2 = u2->next) {
                if (!fval(u2->faction, FFL_SELECT)) {
                    if (cansee(u2->faction, r, u, 0)) {
                        fset(u2->faction, FFL_SELECT);
                        if (!m)
                            m = msg_message("astral_disappear", "unit", u);
                        r_addmessage(r, u2->faction, m);
                    }
                }
            }
            if (m)
                msg_release(m);

            /* Meldungen in der Zielregion */

            for (u2 = rt->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);
            m = NULL;
            for (u2 = rt->units; u2; u2 = u2->next) {
                if (!fval(u2->faction, FFL_SELECT)) {
                    if (cansee(u2->faction, rt, u, 0)) {
                        fset(u2->faction, FFL_SELECT);
                        if (!m)
                            m = msg_message("astral_appear", "unit", u);
                        r_addmessage(rt, u2->faction, m);
                    }
                }
            }
            if (m)
                msg_release(m);
        }
    }
    return cast_level;
}

int sp_leaveastral(castorder * co)
{
    region *rt, *ro;
    region_list *rl, *rl2;
    unit *u, *u2;
    int remaining_cap;
    int n, w;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;

    switch (getplaneid(r)) {
    case 1:
        ro = r;
        rt = pa->param[0]->data.r;
        if (!rt) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "spellfail::noway", ""));
            return 0;
        }
        rl = astralregions(r, inhabitable);
        rl2 = rl;
        while (rl2 != NULL) {
            if (rl2->data == rt)
                break;
            rl2 = rl2->next;
        }
        if (rl2 == NULL) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "spellfail::noway", ""));
            free_regionlist(rl);
            return 0;
        }
        free_regionlist(rl);
        break;
    default:
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spell_astral_only", ""));
        return 0;
    }

    if (ro == NULL || is_cursed(ro->attribs, &ct_astralblock)
        || is_cursed(rt->attribs, &ct_astralblock)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_astralblock", ""));
        return 0;
    }

    remaining_cap = (int)((power - 3) * 1500);

    /* fuer jede Einheit in der Kommandozeile */
    for (n = 1; n < pa->length; n++) {
        if (pa->param[n]->flag == TARGET_NOTFOUND)
            continue;

        u = pa->param[n]->data.u;

        if (!ucontact(u, mage)) {
            if (power > 10 && !(pa->param[n]->flag == TARGET_RESISTS)
                && can_survive(u, rt)) {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "feedback_no_contact_no_resist", "target", u));
                ADDMSG(&u->faction->msgs, msg_message("send_astral", "unit target",
                    mage, u));
            }
            else {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "feedback_no_contact_resist", "target", u));
                ADDMSG(&u->faction->msgs, msg_message("try_astral", "unit target", mage,
                    u));
                continue;
            }
        }

        w = weight(u);

        if (!can_survive(u, rt)) {
            cmistake(mage, co->order, 231, MSG_MAGIC);
        }
        else if (remaining_cap - w < 0) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "fail_tooheavy", "target", u));
        }
        else {
            message *m;

            remaining_cap = remaining_cap - w;
            move_unit(u, rt, NULL);

            /* Meldungen in der Ausgangsregion */

            for (u2 = r->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);
            m = NULL;
            for (u2 = r->units; u2; u2 = u2->next) {
                if (!fval(u2->faction, FFL_SELECT)) {
                    if (cansee(u2->faction, r, u, 0)) {
                        fset(u2->faction, FFL_SELECT);
                        if (!m)
                            m = msg_message("astral_disappear", "unit", u);
                        r_addmessage(r, u2->faction, m);
                    }
                }
            }
            if (m)
                msg_release(m);

            /* Meldungen in der Zielregion */

            for (u2 = rt->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);
            m = NULL;
            for (u2 = rt->units; u2; u2 = u2->next) {
                if (!fval(u2->faction, FFL_SELECT)) {
                    if (cansee(u2->faction, rt, u, 0)) {
                        fset(u2->faction, FFL_SELECT);
                        if (!m)
                            m = msg_message("astral_appear", "unit", u);
                        r_addmessage(rt, u2->faction, m);
                    }
                }
            }
            if (m)
                msg_release(m);
        }
    }
    return cast_level;
}

int sp_fetchastral(castorder * co)
{
    int n;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;
    double power = co->force;
    int remaining_cap = (int)((power - 3) * 1500);
    region_list *rtl = NULL;
    region *rt = co_get_region(co);          /* region to which we are fetching */
    region *ro = NULL;            /* region in which the target is */

    if (rplane(rt)) {
        /* Der Zauber funktioniert nur in der materiellen Welt. */
        cmistake(mage, co->order, 190, MSG_MAGIC);
        return 0;
    }

    /* fuer jede Einheit in der Kommandozeile */
    for (n = 0; n != pa->length; ++n) {
        unit *u2, *u = pa->param[n]->data.u;
        int w;
        message *m;

        if (pa->param[n]->flag & TARGET_NOTFOUND)
            continue;

        if (u->region != ro) {
            /* this can happen several times if the units are from different astral
             * regions. Only possible on the intersections of schemes */
            region_list *rfind;
            if (!is_astral(u->region)) {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "spellfail_astralonly", ""));
                continue;
            }
            if (rtl != NULL)
                free_regionlist(rtl);
            rtl = astralregions(u->region, NULL);
            for (rfind = rtl; rfind != NULL; rfind = rfind->next) {
                if (rfind->data == mage->region)
                    break;
            }
            if (rfind == NULL) {
                /* the region r is not in the schemes of rt */
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "spellfail_distance", "target", u));
                continue;
            }
            ro = u->region;
        }

        if (is_cursed(ro->attribs, &ct_astralblock)) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "spellfail_astralblock", ""));
            continue;
        }

        if (!can_survive(u, rt)) {
            cmistake(mage, co->order, 231, MSG_MAGIC);
            continue;
        }

        w = weight(u);
        if (remaining_cap - w < 0) {
            ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                "fail_tooheavy", "target", u));
            continue;
        }

        if (!ucontact(u, mage)) {
            if (power > 12 && !(pa->param[n]->flag & TARGET_RESISTS)) {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "feedback_no_contact_no_resist", "target", u));
                ADDMSG(&u->faction->msgs, msg_message("send_astral", "unit target",
                    mage, u));
            }
            else {
                ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
                    "feedback_no_contact_resist", "target", u));
                ADDMSG(&u->faction->msgs, msg_message("try_astral", "unit target", mage,
                    u));
                continue;
            }
        }

        remaining_cap -= w;
        move_unit(u, rt, NULL);

        /* Meldungen in der Ausgangsregion */
        for (u2 = ro->units; u2; u2 = u2->next)
            freset(u2->faction, FFL_SELECT);
        m = NULL;
        for (u2 = ro->units; u2; u2 = u2->next) {
            if (!fval(u2->faction, FFL_SELECT)) {
                if (cansee(u2->faction, ro, u, 0)) {
                    fset(u2->faction, FFL_SELECT);
                    if (!m)
                        m = msg_message("astral_disappear", "unit", u);
                    r_addmessage(ro, u2->faction, m);
                }
            }
        }
        if (m)
            msg_release(m);

        /* Meldungen in der Zielregion */
        for (u2 = rt->units; u2; u2 = u2->next)
            freset(u2->faction, FFL_SELECT);
        m = NULL;
        for (u2 = rt->units; u2; u2 = u2->next) {
            if (!fval(u2->faction, FFL_SELECT)) {
                if (cansee(u2->faction, rt, u, 0)) {
                    fset(u2->faction, FFL_SELECT);
                    if (!m)
                        m = msg_message("astral_appear", "unit", u);
                    r_addmessage(rt, u2->faction, m);
                }
            }
        }
        if (m)
            msg_release(m);
    }
    if (rtl != NULL)
        free_regionlist(rtl);
    return cast_level;
}

#ifdef SHOWASTRAL_NOT_BORKED
int sp_showastral(castorder * co)
{
    unit *u;
    region *rt;
    int n = 0;
    int c = 0;
    region_list *rl, *rl2;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;

    switch (getplaneid(r)) {
    case 0:
        rt = r_standard_to_astral(r);
        if (!rt || fval(rt->terrain, FORBIDDEN_REGION)) {
            /* Hier gibt es keine Verbindung zur astralen Welt */
            cmistake(mage, co->order, 216, MSG_MAGIC);
            return 0;
        }
        break;
    case 1:
        rt = r;
        break;
    default:
        /* Hier gibt es keine Verbindung zur astralen Welt */
        cmistake(mage, co->order, 216, MSG_MAGIC);
        return 0;
    }

    rl = all_in_range(rt, power / 5);

    /* Erst Einheiten zaehlen, fuer die Grammatik. */

    for (rl2 = rl; rl2; rl2 = rl2->next) {
        region *r2 = rl2->data;
        if (!is_cursed(r2->attribs, &ct_astralblock)) {
            for (u = r2->units; u; u = u->next) {
                n++;
            }
        }
    }

    if (n == 0) {
        /* sprintf(buf, "%s kann niemanden im astralen Nebel entdecken.",
           unitname(mage)); */
        cmistake(mage, co->order, 220, MSG_MAGIC);
    }
    else {

        /* Ausgeben */

        sprintf(buf, "%s hat eine Vision der astralen Ebene. Im astralen "
            "Nebel zu erkennen sind ", unitname(mage));

        for (rl2 = rl; rl2; rl2 = rl2->next) {
            if (!is_cursed(rl2->data->attribs, &ct_astralblock)) {
                for (u = rl2->data->units; u; u = u->next) {
                    c++;
                    scat(unitname(u));
                    scat(" (");
                    if (!fval(u, UFL_ANON_FACTION)) {
                        scat(factionname(u->faction));
                        scat(", ");
                    }
                    icat(u->number);
                    scat(" ");
                    scat(LOC(mage->faction->locale, rc_name_s(u_race(u), (u->number == 1) ? NAME_SINGULAR : NAME_PLURAL)));
                    scat(", Entfernung ");
                    icat(distance(rl2->data, rt));
                    scat(")");
                    if (c == n - 1) {
                        scat(" und ");
                    }
                    else if (c < n - 1) {
                        scat(", ");
                    }
                }
            }
        }
        scat(".");
        addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);
    }

    free_regionlist(rl);
    return cast_level;
    UNUSED_ARG(co);
    return 0;
}
#endif

/* ------------------------------------------------------------- */
int sp_viewreality(castorder * co)
{
    region_list *rl, *rl2;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    message *m;

    if (getplaneid(r) != 1) {
        /* sprintf(buf, "Dieser Zauber kann nur im Astralraum gezaubert werden."); */
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spell_astral_only", ""));
        return 0;
    }

    rl = astralregions(r, NULL);

    /* Irgendwann mal auf Curses u/o Attribut umstellen. */
    for (rl2 = rl; rl2; rl2 = rl2->next) {
        region *rt = rl2->data;
        if (!is_cursed(rt->attribs, &ct_astralblock)) {
            set_observer(rt, mage->faction, co->level / 2, 2);
        }
    }

    free_regionlist(rl);

    m = msg_message("viewreality_effect", "unit", mage);
    r_addmessage(r, mage->faction, m);
    msg_release(m);

    return cast_level;
}

/* ------------------------------------------------------------- */
int sp_disruptastral(castorder * co)
{
    region_list *rl, *rl2;
    region *rt;
    unit *u;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    int duration = (int)(power / 3) + 1;

    switch (getplaneid(r)) {
    case 0:
        rt = r_standard_to_astral(r);
        if (!rt || fval(rt->terrain, FORBIDDEN_REGION)) {
            /* "Hier gibt es keine Verbindung zur astralen Welt." */
            cmistake(mage, co->order, 216, MSG_MAGIC);
            return 0;
        }
        break;
    case 1:
        rt = r;
        break;
    default:
        /* "Von hier aus kann man die astrale Ebene nicht erreichen." */
        cmistake(mage, co->order, 215, MSG_MAGIC);
        return 0;
    }

    rl = all_in_range(rt, (short)(power / 5), NULL);

    for (rl2 = rl; rl2 != NULL; rl2 = rl2->next) {
        attrib *a;
        double effect;
        region *r2 = rl2->data;
        spec_direction *sd;
        int inhab_regions = 0;
        region_list *trl = NULL;

        if (is_cursed(r2->attribs, &ct_astralblock))
            continue;

        if (r2->units != NULL) {
            region_list *trl2;

            trl = astralregions(rl2->data, inhabitable);
            for (trl2 = trl; trl2; trl2 = trl2->next)
                ++inhab_regions;
        }

        /* Nicht-Permanente Tore zerstoeren */
        a = a_find(r->attribs, &at_direction);

        while (a != NULL && a->type == &at_direction) {
            attrib *a2 = a->next;
            sd = (spec_direction *)(a->data.v);
            if (sd->duration != -1)
                a_remove(&r->attribs, a);
            a = a2;
        }

        /* Einheiten auswerfen */

        if (trl != NULL) {
            for (u = r2->units; u; u = u->next) {
                region_list *trl2 = trl;
                region *tr;
                int c = rng_int() % inhab_regions;

                /* Zufaellige Zielregion suchen */
                while (c-- != 0)
                    trl2 = trl2->next;
                tr = trl2->data;

                if (!is_magic_resistant(mage, u, 0) && can_survive(u, tr)) {
                    message *msg = msg_message("disrupt_astral", "unit region", u, tr);
                    add_message(&u->faction->msgs, msg);
                    add_message(&tr->msgs, msg);
                    msg_release(msg);

                    move_unit(u, tr, NULL);
                }
            }
            free_regionlist(trl);
        }

        /* Kontakt unterbinden */
        effect = 100;
        create_curse(mage, &rl2->data->attribs, &ct_astralblock,
            power, duration, effect, 0);
    }

    free_regionlist(rl);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Mauern der Ewigkeit
 * Stufe:      7
 * Kategorie:  Artefakt
 * Gebiet:     Tybied
 * Wirkung:
 * Das Gebaeude kostet keinen Unterhalt mehr
 *
 * ZAUBER "Mauern der Ewigkeit" <gebaeude-nummer>
 * Flags: (0)
 */
static int sp_eternizewall(castorder * co)
{
    unit *u;
    curse *c;
    building *b;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;
    message *msg;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    b = pa->param[0]->data.b;
    c = create_curse(mage, &b->attribs, &ct_nocostbuilding,
        power * power, 1, zero_effect, 0);

    if (c == NULL) {              /* ist bereits verzaubert */
        cmistake(mage, co->order, 206, MSG_MAGIC);
        return 0;
    }

    /* melden, 1x pro Partei in der Burg */
    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    msg =
        msg_message("sp_eternizewall_effect", "mage building region", mage, b, r);
    for (u = r->units; u; u = u->next) {
        if (!fval(u->faction, FFL_SELECT)) {
            fset(u->faction, FFL_SELECT);
            if (u->building == b) {
                r_addmessage(r, u->faction, msg);
            }
        }
    }
    if (r != mage->region) {
        add_message(&mage->faction->msgs, msg);
    }
    else if (mage->building != b) {
        r_addmessage(r, mage->faction, msg);
    }
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Opfere Kraft
 * Stufe:      15
 * Gebiet:     Tybied
 * Kategorie:  Einheit, positiv
 * Wirkung:
 *   Mit Hilfe dieses Zaubers kann der Magier einen Teil seiner
 *   magischen Kraft permanent auf einen anderen Magier uebertragen. Auf
 *   einen Tybied-Magier kann er die Haelfte der eingesetzten Kraft
 *   uebertragen, auf einen Magier eines anderen Gebietes ein Drittel.
 *
 * Flags:
 * (UNITSPELL)
 *
 * Syntax:
 * ZAUBERE \"Opfere Kraft\" <Einheit-Nr> <Aura>
 * "ui"
 */
int sp_permtransfer(castorder * co)
{
    int aura;
    unit *tu;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;
    const spell *sp = co->sp;
    message *msg;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
     * abbrechen aber kosten lassen */
    if (pa->param[0]->flag == TARGET_RESISTS)
        return cast_level;

    tu = pa->param[0]->data.u;
    aura = pa->param[1]->data.i;

    if (!is_mage(tu)) {
        /*    sprintf(buf, "%s in %s: 'ZAUBER %s': Einheit ist kein Magier."
              , unitname(mage), regionname(mage->region, mage->faction),sa->strings[0]); */
        cmistake(mage, co->order, 214, MSG_MAGIC);
        return 0;
    }

    aura = MIN(get_spellpoints(mage) - spellcost(mage, sp), aura);

    change_maxspellpoints(mage, -aura);
    change_spellpoints(mage, -aura);

    if (get_mage(tu)->magietyp == get_mage(mage)->magietyp) {
        change_maxspellpoints(tu, aura / 2);
    }
    else {
        change_maxspellpoints(tu, aura / 3);
    }

    msg =
        msg_message("sp_permtransfer_effect", "mage target amount", mage, tu, aura);
    add_message(&mage->faction->msgs, msg);
    if (tu->faction != mage->faction) {
        add_message(&tu->faction->msgs, msg);
    }
    msg_release(msg);

    return cast_level;
}

/* ------------------------------------------------------------- */
/* TODO: specialdirections? */

int sp_movecastle(castorder * co)
{
    building *b;
    direction_t dir;
    region *target_region;
    unit *u, *unext;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;
    message *msg;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    b = pa->param[0]->data.b;
    dir = get_direction(pa->param[1]->data.xs, mage->faction->locale);

    if (dir == NODIRECTION) {
        /* Die Richtung wurde nicht erkannt */
        cmistake(mage, co->order, 71, MSG_PRODUCE);
        return 0;
    }

    if (b->size > (cast_level - 12) * 250) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "sp_movecastle_fail_0", ""));
        return cast_level;
    }

    target_region = rconnect(r, dir);

    if (!target_region || !(target_region->terrain->flags & LAND_REGION)) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "sp_movecastle_fail_1", "direction", dir));
        return cast_level;
    }

    bunhash(b);
    translist(&r->buildings, &target_region->buildings, b);
    b->region = target_region;
    b->size -= b->size / (10 - rng_int() % 6);
    bhash(b);

    for (u = r->units; u;) {
        unext = u->next;
        if (u->building == b) {
            uunhash(u);
            translist(&r->units, &target_region->units, u);
            uhash(u);
        }
        u = unext;
    }

    if ((is_building_type(b->type, "caravan") || is_building_type(b->type, "dam")
        || is_building_type(b->type, "tunnel"))) {
        direction_t d;
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            if (rroad(r, d)) {
                rsetroad(r, d, rroad(r, d) / 2);
            }
        }
    }

    msg = msg_message("sp_movecastle_effect", "building direction", b, dir);
    r_addmessage(r, NULL, msg);
    msg_release(msg);

    return cast_level;
}


/* ------------------------------------------------------------- */
/* Name:       Stehle Aura
 * Stufe:      6
 * Kategorie:  Einheit, negativ
 * Wirkung:
 *     Mit Hilfe dieses Zaubers kann der Magier einem anderen Magier
 *     seine Aura gegen dessen Willen entziehen und sich selber
 *     zufuehren.
 *
 * Flags:
 * (FARCASTING | SPELLLEVEL | UNITSPELL | TESTRESISTANCE |
 * TESTCANSEE)
 * */
int sp_stealaura(castorder * co)
{
    int taura;
    unit *u;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double power = co->force;
    spellparameter *pa = co->par;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    /* Zieleinheit */
    u = pa->param[0]->data.u;

    if (!get_mage(u)) {
        ADDMSG(&mage->faction->msgs, msg_message("stealaura_fail", "unit target",
            mage, u));
        ADDMSG(&u->faction->msgs, msg_message("stealaura_fail_detect", "unit", u));
        return 0;
    }

    taura = (get_mage(u)->spellpoints * (rng_int() % (int)(3 * power) + 1)) / 100;

    if (taura > 0) {
        get_mage(u)->spellpoints -= taura;
        get_mage(mage)->spellpoints += taura;
        /*    sprintf(buf, "%s entzieht %s %d Aura.", unitname(mage), unitname(u),
              taura); */
        ADDMSG(&mage->faction->msgs, msg_message("stealaura_success",
            "mage target aura", mage, u, taura));
        /*    sprintf(buf, "%s fuehlt seine magischen Kraefte schwinden und verliert %d "
              "Aura.", unitname(u), taura); */
        ADDMSG(&u->faction->msgs, msg_message("stealaura_detect", "unit aura", u,
            taura));
    }
    else {
        ADDMSG(&mage->faction->msgs, msg_message("stealaura_fail", "unit target",
            mage, u));
        ADDMSG(&u->faction->msgs, msg_message("stealaura_fail_detect", "unit", u));
    }
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:       Astrale Schwaechezone
 * Stufe:      5
 * Kategorie:
 * Wirkung:
 *    Reduziert die Staerke jedes Spruch in der Region um Level Haelt
 *    Sprueche bis zu einem Gesammtlevel von Staerke*10 aus, danach ist
 *    sie verbraucht.
 *    leibt bis zu Staerke Wochen aktiv.
 *    Ein Ring der Macht erhoeht die Staerke um 1, in einem Magierturm
 *    gezaubert gibt nochmal +1 auf Staerke. (force)
 *
 *    Beispiel:
 *    Eine Antimagiezone Stufe 7 haelt bis zu 7 Wochen an oder Sprueche mit
 *    einem Gesammtlevel bis zu 70 auf. Also zB 7 Stufe 10 Sprueche, 10
 *    Stufe 7 Sprueche oder 35 Stufe 2 Sprueche.  Sie reduziert die Staerke
 *    (level+boni) jedes Spruchs, der in der Region gezaubert wird, um
 *    7. Alle Sprueche mit einer Staerke kleiner als 7 schlagen fehl
 *    (power = 0).
 *
 * Flags:
 * (FARCASTING | SPELLLEVEL | REGIONSPELL | TESTRESISTANCE)
 *
 */
int sp_antimagiczone(castorder * co)
{
    double power;
    double effect;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    int duration = (int)force + 1;

    /* Haelt Sprueche bis zu einem summierten Gesamtlevel von power aus.
     * Jeder Zauber reduziert die 'Lebenskraft' (vigour) der Antimagiezone
     * um seine Stufe */
    power = force * 10;

    /* Reduziert die Staerke jedes Spruchs um effect */
    effect = cast_level;

    create_curse(mage, &r->attribs, &ct_antimagiczone, power, duration,
        effect, 0);

    /* Erfolg melden */
    ADDMSG(&mage->faction->msgs, msg_message("regionmagic_effect",
        "unit region command", mage, mage->region, co->order));

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Schutzrunen
 * Stufe:   8
 * Kosten:   SPC_FIX
 *
 * Wirkung:
 *   Gibt Gebaeuden einen Bonus auf Magieresistenz von +20%. Der Zauber
 *   dauert 3+rng_int()%Level Wochen an, also im Extremfall bis zu 2 Jahre
 *   bei Stufe 20
 *
 *   Es koennen mehrere Zauber uebereinander gelegt werden, der Effekt
 *   summiert sich, jedoch wird die Dauer dadurch nicht verlaengert.
 *
 * oder:
 *
 *   Gibt Schiffen einen Bonus auf Magieresistenz von +20%. Der Zauber
 *   dauert 3+rng_int()%Level Wochen an, also im Extremfall bis zu 2 Jahre
 *   bei Stufe 20
 *
 *   Es koennen mehrere Zauber uebereinander gelegt werden, der Effekt
 *   summiert sich, jedoch wird die Dauer dadurch nicht verlaengert.
 *
 * Flags:
 * (ONSHIPCAST | TESTRESISTANCE)
 *
 * Syntax:
 *  ZAUBERE \"Runen des Schutzes\" GEBAEUDE <Gebaeude-Nr>
 *  ZAUBERE \"Runen des Schutzes\" SCHIFF <Schiff-Nr>
 * "kc"
 */

static int sp_magicrunes(castorder * co)
{
    int duration;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    spellparameter *pa = co->par;
    double effect;

    duration = 3 + rng_int() % cast_level;
    effect = 20;

    switch (pa->param[0]->typ) {
    case SPP_BUILDING:
    {
        building *b;
        b = pa->param[0]->data.b;

        /* Magieresistenz der Burg erhoeht sich um 20% */
        create_curse(mage, &b->attribs, &ct_magicrunes, force,
            duration, effect, 0);

        /* Erfolg melden */
        ADDMSG(&mage->faction->msgs, msg_message("objmagic_effect",
            "unit region command target", mage, mage->region, co->order,
            buildingname(b)));
        break;
    }
    case SPP_SHIP:
    {
        ship *sh;
        sh = pa->param[0]->data.sh;
        /* Magieresistenz des Schiffs erhoeht sich um 20% */
        create_curse(mage, &sh->attribs, &ct_magicrunes, force,
            duration, effect, 0);

        /* Erfolg melden */
        ADDMSG(&mage->faction->msgs, msg_message("objmagic_effect",
            "unit region command target", mage, mage->region, co->order,
            shipname(sh)));
        break;
    }
    default:
        /* fehlerhafter Parameter */
        return 0;
    }

    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:   Zeitdehnung
 *
 * Flags:
 * (UNITSPELL | SPELLLEVEL | ONSHIPCAST | TESTCANSEE)
 * Syntax:
 *  "u+"
 */

int sp_speed2(castorder * co)
{
    int n, maxmen, used = 0, dur, men;
    unit *u;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    spellparameter *pa = co->par;

    maxmen = 2 * cast_level * cast_level;
    dur = MAX(1, cast_level / 2);

    for (n = 0; n < pa->length; n++) {
        double effect;
        /* sollte nie negativ werden */
        if (maxmen < 1)
            break;

        if (pa->param[n]->flag == TARGET_RESISTS
            || pa->param[n]->flag == TARGET_NOTFOUND)
            continue;

        u = pa->param[n]->data.u;

        men = MIN(maxmen, u->number);
        effect = 2;
        create_curse(mage, &u->attribs, &ct_speed, force, dur, effect, men);
        maxmen -= men;
        used += men;
    }

    ADDMSG(&mage->faction->msgs, msg_message("speed_time_effect",
        "unit region amount", mage, mage->region, used));
    /* Effektiv benoetigten cast_level (mindestens 1) zurueckgeben */
    used = (int)sqrt(used / 2);
    return MAX(1, used);
}

/* ------------------------------------------------------------- */
/* Name:     Magiefresser
 * Stufe:   7
 * Kosten:   SPC_LEVEL
 *
 * Wirkung:
 *   Kann eine bestimmte Verzauberung angreifen und aufloesen. Die Staerke
 *   des Zaubers muss staerker sein als die der Verzauberung.
 * Syntax:
 *  ZAUBERE \"Magiefresser\" REGION
 *  ZAUBERE \"Magiefresser\" EINHEIT <Einheit-Nr>
 *  ZAUBERE \"Magiefresser\" GEBAEUDE <Gebaeude-Nr>
 *  ZAUBERE \"Magiefresser\" SCHIFF <Schiff-Nr>
 *
 *  "kc?c"
 * Flags:
 *   (FARCASTING | SPELLLEVEL | ONSHIPCAST | TESTCANSEE)
 */
 /* Jeder gebrochene Zauber verbraucht c->vigour an Zauberkraft
  * (force) */
int sp_q_antimagie(castorder * co)
{
    attrib **ap;
    int obj;
    curse *c = NULL;
    int succ;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    spellparameter *pa = co->par;
    const char *ts = NULL;

    obj = pa->param[0]->typ;

    switch (obj) {
    case SPP_REGION:
        ap = &r->attribs;
        ts = regionname(r, mage->faction);
        break;

    case SPP_TEMP:
    case SPP_UNIT:
    {
        unit *u = pa->param[0]->data.u;
        ap = &u->attribs;
        ts = itoa36(u->no);
        break;
    }
    case SPP_BUILDING:
    {
        building *b = pa->param[0]->data.b;
        ap = &b->attribs;
        ts = itoa36(b->no);
        break;
    }
    case SPP_SHIP:
    {
        ship *sh = pa->param[0]->data.sh;
        ap = &sh->attribs;
        ts = itoa36(sh->no);
        break;
    }
    default:
        /* Das Zielobjekt wurde vergessen */
        cmistake(mage, co->order, 203, MSG_MAGIC);
        return 0;
    }

    succ = break_curse(ap, cast_level, force, c);

    if (succ) {
        ADDMSG(&mage->faction->msgs, msg_message("destroy_magic_effect",
            "unit region command succ target", mage, mage->region, co->order, succ,
            ts));
    }
    else {
        ADDMSG(&mage->faction->msgs, msg_message("destroy_magic_noeffect",
            "unit region command", mage, mage->region, co->order));
    }
    return MAX(succ, 1);
}

/* ------------------------------------------------------------- */
/* Name:     Fluch brechen
 * Stufe:   7
 * Kosten:   SPC_LEVEL
 *
 * Wirkung:
 *   Kann eine bestimmte Verzauberung angreifen und aufloesen. Die Staerke
 *   des Zaubers muss staerker sein als die der Verzauberung.
 * Syntax:
 *  ZAUBERE \"Fluch brechen\" REGION <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" EINHEIT <Einheit-Nr> <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" GEBAEUDE <Gebaeude-Nr> <Zauber-id>
 *  ZAUBERE \"Fluch brechen\" SCHIFF <Schiff-Nr> <Zauber-id>
 *
 *  "kcc"
 * Flags:
 *   (FARCASTING | SPELLLEVEL | ONSHIPCAST | TESTCANSEE)
 */
int sp_break_curse(castorder * co)
{
    attrib **ap;
    int obj;
    curse *c;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    double force = co->force;
    spellparameter *pa = co->par;
    const char *ts = NULL;

    if (pa->length < 2) {
        /* Das Zielobjekt wurde vergessen */
        cmistake(mage, co->order, 203, MSG_MAGIC);
        return 0;
    }

    obj = pa->param[0]->typ;

    c = findcurse(atoi36(pa->param[1]->data.s));
    if (!c) {
        /* Es wurde kein Ziel gefunden */
        ADDMSG(&mage->faction->msgs, msg_message("spelltargetnotfound",
            "unit region command", mage, mage->region, co->order));
    }
    else {
        switch (obj) {
        case SPP_REGION:
            ap = &r->attribs;
            ts = regionname(r, mage->faction);
            break;

        case SPP_TEMP:
        case SPP_UNIT:
        {
            unit *u = pa->param[0]->data.u;
            ap = &u->attribs;
            ts = itoa36(u->no);
            break;
        }
        case SPP_BUILDING:
        {
            building *b = pa->param[0]->data.b;
            ap = &b->attribs;
            ts = itoa36(b->no);
            break;
        }
        case SPP_SHIP:
        {
            ship *sh = pa->param[0]->data.sh;
            ap = &sh->attribs;
            ts = itoa36(sh->no);
            break;
        }
        default:
            /* Das Zielobjekt wurde vergessen */
            cmistake(mage, co->order, 203, MSG_MAGIC);
            return 0;
        }

        /* ueberpruefung, ob curse zu diesem objekt gehoert */
        if (!is_cursed_with(*ap, c)) {
            /* Es wurde kein Ziel gefunden */
            ADDMSG(&mage->faction->msgs,
                msg_message("spelltargetnotfound", "unit region command",
                    mage, mage->region, co->order));
        }

        /* curse aufloesen, wenn zauber staerker (force > vigour) */
        c->vigour -= force;

        if (c->vigour <= 0.0) {
            remove_curse(ap, c);

            ADDMSG(&mage->faction->msgs, msg_message("destroy_curse_effect",
                "unit region command id target", mage, mage->region, co->order,
                pa->param[1]->data.xs, ts));
        }
        else {
            ADDMSG(&mage->faction->msgs, msg_message("destroy_curse_noeffect",
                "unit region command id target", mage, mage->region, co->order,
                pa->param[1]->data.xs, ts));
        }
    }

    return cast_level;
}

static int sp_flee(castorder *co) {
    if (co->force <= 0) {
        return 0;
    }
    return flee_spell(co, 4);
}

static int sp_song_of_fear(castorder *co) {
    if (co->force <= 0) {
        return 0;
    }
    return flee_spell(co, 3);
}

static int sp_aura_of_fear(castorder *co) {
    if (co->force <= 0) {
        return 0;
    }
    return flee_spell(co, 5);
}

static int sp_armor_shield(struct castorder * co) {
    return armor_spell(co, 3, 20);
}

static int sp_bark_skin(struct castorder * co) {
    return armor_spell(co, 4, 1);
}

static int sp_kampfzauber(castorder *co) {
    const spell * sp = co->sp;
    if (co->force <= 0) {
        return 0;
    }
    else if (strcmp(sp->sname, "fireball") == 0) {
        return damage_spell(co, 0, 0);
    }
    else if (strcmp(sp->sname, "hail") == 0) {
        return damage_spell(co, 2, 4);
    }
    else if (strcmp(sp->sname, "meteor_rain") == 0) {
        return damage_spell(co, 1, 1);
    }
    else {
        return damage_spell(co, 10, 10);
    }
}

/* ------------------------------------------------------------- */
int sp_becomewyrm(castorder * co)
{
    UNUSED_ARG(co);
    return 0;
}

/* ------------------------------------------------------------- */
/* Name:       Plappermaul
* Stufe:      4
* Gebiet:     Cerddor
* Kategorie:  Einheit
*
* Wirkung:
*  Einheit ausspionieren. Gibt auch Zauber und Kampfstatus aus.  Wirkt
*  gegen Magieresistenz. Ist diese zu hoch, so wird der Zauber entdeckt
*  (Meldung) und der Zauberer erhaelt nur die Talente, nicht die Werte
*  der Einheit und auch keine Zauber.
*
* Flag:
*  (UNITSPELL | TESTCANSEE)
*/
static int sp_babbler(castorder * co)
{
    unit *target;
    region *r = co_get_region(co);
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;
    message *msg;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    target = pa->param[0]->data.u;

    if (target->faction == mage->faction) {
        /* Die Einheit ist eine der unsrigen */
        cmistake(mage, co->order, 45, MSG_MAGIC);
    }

    /* Magieresistenz Unit */
    if (target_resists_magic(mage, target, TYP_UNIT, 0)) {
        spy_message(5, mage, target);
        msg = msg_message("babbler_resist", "unit mage", target, mage);
    }
    else {
        spy_message(100, mage, target);
        msg = msg_message("babbler_effect", "unit", target);
    }
    r_addmessage(r, target->faction, msg);
    msg_release(msg);
    return cast_level;
}

/* ------------------------------------------------------------- */
/* Name:     Traumdeuten
* Stufe:   7
* Kategorie:      Einheit
*
* Wirkung:
*  Wirkt gegen Magieresistenz.  Spioniert die Einheit aus. Gibt alle
*  Gegenstaende, Talente mit Stufe, Zauber und Kampfstatus an.
*
*  Magieresistenz hier pruefen, wegen Fehlermeldung
*
* Flag:
* (UNITSPELL)
*/
static int sp_readmind(castorder * co)
{
    unit *target;
    unit *mage = co->magician.u;
    int cast_level = co->level;
    spellparameter *pa = co->par;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (pa->param[0]->flag == TARGET_NOTFOUND)
        return 0;

    target = pa->param[0]->data.u;

    if (target->faction == mage->faction) {
        /* Die Einheit ist eine der unsrigen */
        cmistake(mage, co->order, 45, MSG_MAGIC);
    }

    /* Magieresistenz Unit */
    if (target_resists_magic(mage, target, TYP_UNIT, 0)) {
        cmistake(mage, co->order, 180, MSG_MAGIC);
        /* "Fuehlt sich beobachtet" */
        ADDMSG(&target->faction->msgs, msg_message("stealdetect", "unit", target));
        return 0;
    }
    spy_message(2, mage, target);

    return cast_level;
}

typedef struct spelldata {
    const char *sname;
    spell_f cast;
    fumble_f fumble;
} spelldata;

static spelldata spell_functions[] = {
    /* M_GWYRRD */
    { "stonegolem", sp_create_stonegolem, 0 },
    { "irongolem", sp_create_irongolem, 0 },
    { "treegrow", sp_hain, fumble_ents },
    { "rustweapon", sp_rosthauch, 0 },
    { "cold_protection", sp_kaelteschutz, 0 },
    { "ironkeeper", sp_ironkeeper, 0 },
    { "magicstreet", sp_magicstreet, 0 },
    { "windshield", sp_windshield, 0 },
    { "mallorntreegrow", sp_mallornhain, fumble_ents },
    { "goodwinds", sp_goodwinds, 0 },
    { "healing", sp_healing, 0 },
    { "reelingarrows", sp_reeling_arrows, 0 },
    { "gwyrrdfumbleshield", sp_fumbleshield, 0 },
    { "transferauradruide", sp_transferaura, 0 },
    { "earthquake", sp_earthquake, 0 },
    { "stormwinds", sp_stormwinds, 0 },
    { "homestone", sp_homestone, 0 },
    { "wolfhowl", sp_wolfhowl, 0 },
    { "igjarjuk", sp_igjarjuk, 0 },
    { "versteinern", sp_petrify, 0 },
    { "strongwall", sp_strong_wall, 0 },
    { "gwyrrddestroymagic", sp_destroy_magic, 0 },
    { "treewalkenter", sp_treewalkenter, 0 },
    { "treewalkexit", sp_treewalkexit, 0 },
    { "holyground", sp_holyground, 0 },
    { "summonent", sp_summonent, 0 },
    { "blessstonecircle", sp_blessstonecircle, 0 },
    { "barkskin", sp_bark_skin, 0 },
    { "summonfireelemental", sp_drought, 0 },
    { "maelstrom", sp_maelstrom, 0 },
    { "magic_roots", sp_mallorn, 0 },
    { "great_drought", sp_great_drought, 0 },
    /* M_DRAIG */
    { "sparklechaos", sp_sparkle, 0 },
    { "magicboost", sp_magicboost, 0 },
    { "bloodsacrifice", sp_bloodsacrifice, 0 },
    { "berserk", sp_berserk, 0 },
    { "fumblecurse", sp_fumblecurse, patzer_fumblecurse },
    { "summonundead", sp_summonundead, patzer_peasantmob },
    { "combatrust", sp_combatrosthauch, 0 },
    { "transferaurachaos", sp_transferaura, 0 },
    { "firewall", sp_firewall, patzer_peasantmob },
    { "plague", sp_plague, patzer_peasantmob },
    { "chaosrow", sp_chaosrow, 0 },
    { "summonshadow", sp_summonshadow, patzer_peasantmob },
    { "undeadhero", sp_undeadhero, 0 },
    { "auraleak", sp_auraleak, 0 },
    { "draigfumbleshield", sp_fumbleshield, 0 },
    { "forestfire", sp_forest_fire, patzer_peasantmob },
    { "draigdestroymagic", sp_destroy_magic, 0 },
    { "unholypower", sp_unholypower, 0 },
    { "deathcloud", sp_deathcloud, patzer_peasantmob },
    { "summondragon", sp_summondragon, patzer_peasantmob },
    { "summonshadowlords", sp_summonshadowlords, patzer_peasantmob },
    { "chaossuction", sp_chaossuction, patzer_peasantmob },
    /* M_ILLAUN */
    { "sparkledream", sp_sparkle, 0 },
    { "shadowknights", sp_shadowknights, 0 },
    { "flee", sp_flee, 0 },
    { "puttorest", sp_puttorest, 0 },
    { "icastle", sp_icastle, 0 },
    { "transferauratraum", sp_transferaura, 0 },
    { "shapeshift", sp_illusionary_shapeshift, 0 },
    { "dreamreading", sp_dreamreading, 0 },
    { "tiredsoldiers", sp_tiredsoldiers, 0 },
    { "reanimate", sp_reanimate, 0 },
    { "analysedream", sp_analysedream, 0 },
    { "disturbingdreams", sp_disturbingdreams, 0 },
    { "sleep", sp_sleep, 0 },
    { "wisps", 0, 0 }, /* TODO: this spell is gone */
    { "gooddreams", sp_gooddreams, 0 },
    { "illaundestroymagic", sp_destroy_magic, 0 },
    { "clone", sp_clonecopy, 0 },
    { "bad_dreams", sp_baddreams, 0 },
    { "mindblast", sp_mindblast_temp, 0 },
    { "orkdream", sp_sweetdreams, 0 },
    /* M_CERDDOR */
    { "appeasement", sp_denyattack, 0 },
    { "song_of_healing", sp_healing, 0 },
    { "generous", sp_generous, 0 },
    { "song_of_fear", sp_song_of_fear, 0 },
    { "courting", sp_recruit, 0 },
    { "song_of_confusion", sp_chaosrow, 0 },
    { "heroic_song", sp_hero, 0 },
    { "transfer_aura_song", sp_transferaura, 0 },
    { "analysesong_unit", sp_analysesong_unit, 0 },
    { "cerddorfumbleshield", sp_fumbleshield, 0 },
    { "calm_monster", sp_calm_monster, 0 },
    { "seduction", sp_seduce, 0 },
    { "headache", sp_headache, 0 },
    { "sound_out", sp_pump, 0 },
    { "bloodthirst", sp_berserk, 0 },
    { "frighten", sp_frighten, 0 },
    { "analyse_object", sp_analysesong_obj, 0 },
    { "cerddor_destroymagic", sp_destroy_magic, 0 },
    { "migration", sp_migranten, 0 },
    { "summon_familiar", sp_summon_familiar, 0 },
    { "raise_mob", sp_raisepeasants, 0 },
    { "song_resist_magic", sp_song_resistmagic, 0 },
    { "melancholy", sp_depression, 0 },
    { "song_suscept_magic", sp_song_susceptmagic, 0 },
    { "song_of_peace", sp_song_of_peace, 0 },
    { "song_of_slavery", sp_charmingsong, 0 },
    { "big_recruit", sp_bigrecruit, 0 },
    { "calm_riot", sp_rallypeasantmob, 0 },
    { "incite_riot", sp_raisepeasantmob, 0 },
    /* M_TYBIED */
    { "analyze_magic", sp_analysemagic, 0 },
    { "concealing_aura", sp_itemcloak, 0 },
    { "tybiedfumbleshield", sp_fumbleshield, 0 },
#ifdef SHOWASTRAL_NOT_BORKED
    { "show_astral", sp_showastral, 0 },
#endif
    { "resist_magic", sp_resist_magic_bonus, 0 },
    { "keeploot", sp_keeploot, 0 },
    { "enterastral", sp_enterastral, 0 },
    { "leaveastral", sp_leaveastral, 0 },
    { "auratransfer", sp_transferaura, 0 },
    { "shockwave", sp_stun, 0 },
    { "antimagiczone", sp_antimagiczone, 0 },
    { "destroy_magic", sp_destroy_magic, 0 },
    { "pull_astral", sp_pullastral, 0 },
    { "fetch_astral", sp_fetchastral, 0 },
    { "steal_aura", sp_stealaura, 0 },
    { "airship", sp_flying_ship, 0 },
    { "break_curse", sp_break_curse, 0 },
    { "eternal_walls", sp_eternizewall, 0 },
    { "protective_runes", sp_magicrunes, 0 },
    { "fish_shield", sp_reduceshield, 0 },
    { "combat_speed", sp_speed, 0 },
    { "view_reality", sp_viewreality, 0 },
    { "double_time", sp_speed2, 0 },
    { "armor_shield", sp_armor_shield, 0 },
    { "living_rock", sp_movecastle, 0 },
    { "astral_disruption", sp_disruptastral, 0 },
    { "sacrifice_strength", sp_permtransfer, 0 },
    /* M_GRAY */
    /*  Definitionen von Create_Artefaktspruechen    */
    { "wyrm_transformation", sp_becomewyrm, 0 },
    /* Monstersprueche */
    { "fiery_dragonbreath", sp_dragonodem, 0 },
    { "icy_dragonbreath", sp_dragonodem, 0 },
    { "powerful_dragonbreath", sp_dragonodem, 0 },
    { "drain_skills", sp_dragonodem, 0 },
    { "aura_of_fear", sp_aura_of_fear, 0 },
    { "immolation", sp_immolation, 0 },
    { "firestorm", sp_immolation, 0 },
    { "coldfront", sp_immolation, 0 },
    { "acidrain", sp_immolation, 0 },
    { "blabbermouth", sp_babbler, NULL },
    { "summon_familiar", sp_summon_familiar, NULL },
    { "meteor_rain", sp_kampfzauber, NULL },
    { "fireball", sp_kampfzauber, NULL },
    { "hail", sp_kampfzauber, NULL },
    { "readmind", sp_readmind, NULL },
    { "blessedharvest", sp_blessedharvest, NULL },
    { "raindance", sp_blessedharvest, NULL },
    { 0, 0, 0 }
};

static void register_spelldata(void)
{
    int i;
    for (i = 0; spell_functions[i].sname; ++i) {
        spelldata *data = spell_functions + i;
        if (data->cast) {
            add_spellcast(data->sname, data->cast);
        }
        if (data->fumble) {
            add_fumble(data->sname, data->fumble);
        }
    }
}

void register_spells(void)
{
    register_borders();

#ifdef COMPAT_DEATHCLOUD
    at_deprecate("zauber_todeswolke", dc_read_compat);
#endif
    
    /* init_firewall(); */
    ct_register(&ct_firewall);
    ct_register(&ct_deathcloud);

    register_spelldata();

    register_unitcurse();
    register_regioncurse();
    register_shipcurse();
    register_buildingcurse();
    register_magicresistance();

    register_flyingship();
}
