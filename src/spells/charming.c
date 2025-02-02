#include "charming.h"

#include <magic.h>

#include <triggers/changefaction.h>
#include <triggers/killunit.h>
#include <triggers/timeout.h>

#include <kernel/curse.h>
#include <kernel/event.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/objtypes.h>
#include <kernel/skill.h>
#include <kernel/skills.h>
#include <kernel/unit.h>

#include <util/message.h>
#include <util/rng.h>

#include <storage.h>

#include <stb_ds.h>

/* libc includes */
#include <assert.h>
#include <limits.h>

/* ------------------------------------------------------------- */
/*
 * C_SLAVE
 */
static message *cinfo_slave(const void *obj, objtype_t typ, const curse *c,
    int self)
{
    unit *u;
    (void) typ;

    assert(typ == TYP_UNIT);
    u = (unit *)obj;

    if (self != 0) {
        return msg_message("curseinfo::slave_1", "unit duration id", u, c->duration,
            c->no);
    }
    return NULL;
}

const struct curse_type ct_slavery = { 
    "slavery", CURSETYP_NORM, 0, NO_MERGE, cinfo_slave
};

static bool can_charm(const unit *u, int maxlevel)
{
    const skill_t expskills[] =
    { SK_ALCHEMY, SK_HERBALISM, SK_MAGIC, SK_SPY, SK_TACTICS, NOSKILL };
    size_t s, len;

    if (u->flags & UFL_HERO) {
        return false;
    }

    for (len = arrlen(u->skills), s = 0; s != len; ++s) {
        skill *sv = u->skills + s;
        int l = 0, h = 5;
        skill_t sk = sv->id;
        assert(expskills[h] == NOSKILL);
        while (l < h) {
            int m = (l + h) / 2;
            if (sk == expskills[m]) {
                if (faction_skill_limit(u->faction, sk) != INT_MAX) {
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

void charm_unit(unit *target, unit *mage, double force, int duration)
{
    trigger *trestore = trigger_changefaction(target, target->faction);
    /* laeuft die Dauer ab, setze Partei zurueck */
    add_trigger(&target->attribs, "timer", trigger_timeout(duration, trestore));
    /* wird die alte Partei von Target aufgeloest, dann auch diese Einheit */
    add_trigger(&target->faction->attribs, "destroy", trigger_killunit(target));
    /* wird die neue Partei von Target aufgeloest, dann auch diese Einheit */
    add_trigger(&mage->faction->attribs, "destroy", trigger_killunit(target));
    /* sperre ATTACKIERE, GIB PERSON und ueberspringe Migranten */
    create_curse(mage, &target->attribs, &ct_slavery, force, duration, 0.0, 0);

    /* setze Partei um und loesche langen Befehl aus Sicherheitsgruenden */
    u_setfaction(target, mage->faction);
    u_freeorders(target);

    /* setze Parteitarnung, damit nicht sofort klar ist, wer dahinter
     * steckt */
    if (rule_stealth_anon()) {
        target->flags |= UFL_ANON_FACTION;
    }
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
int sp_charmingsong(castorder *co)
{
    unit *target;
    skill_t i;
    unit *mage = co_get_caster(co);
    int cast_level = co->level;
    double force = co->force;
    spellparameter *param = co->a_params;
    int duration, resist_bonus = 0;
    int tb = 0;

    /* wenn Ziel gefunden, dieses aber Magieresistent war, Zauber
     * abbrechen aber kosten lassen */
    if (param->flag == TARGET_RESISTS)
        return cast_level;

    /* wenn kein Ziel gefunden, Zauber abbrechen */
    if (param->flag == TARGET_NOTFOUND)
        return 0;

    target = param->data.u;

    /* Auf eigene Einheiten versucht zu zaubern? Garantiert Tippfehler */
    if (target->faction == mage->faction) {
        /* Die Einheit ist eine der unsrigen */
        cmistake(mage, co->order, 45, MSG_MAGIC);
        return 0;
    }
    /* niemand mit teurem Talent */
    if (!can_charm(target, (int)(force / 2))) {
        ADDMSG(&mage->faction->msgs, msg_feedback(mage, co->order,
            "spellfail_noexpensives", "target", target));
        return co->level;
    }

    /* Magieresistensbonus fuer mehr als Stufe Personen */
    if (target->number > force) {
        resist_bonus += (int)((target->number - force) * 10);
    }
    /* Magieresistensbonus fuer hoehere Talentwerte */
    for (i = 0; i < MAXSKILLS; i++) {
        int sk = effskill(target, i, NULL);
        if (tb < sk)
            tb = sk;
    }
    if (tb > 0) {
        tb -= effskill(mage, SK_MAGIC, NULL);
        if (tb > 0) {
            resist_bonus += tb * 15;
        }
    }
    /* Increased chance for magical resistence */
    if (resist_bonus > 0) {
        variant p_regular = resist_chance(mage, target, TYP_UNIT, 0);
        variant p_modified = resist_chance(mage, target, TYP_UNIT, resist_bonus);
        variant prob = frac_div(p_regular, p_modified);
        if (prob.sa[0] > 0) {
            if (rng_int() % prob.sa[1] < prob.sa[0]) {
                /* target resists after all, because of bonus */
                ADDMSG(&mage->faction->msgs, msg_message("spellunitresists",
                    "unit region command target", mage, mage->region, co->order, target));
                return cast_level;
            }
        }
    }

    duration = 3 + rng_int() % (int)force;
    charm_unit(target, mage, force, duration);
    ADDMSG(&mage->faction->msgs, msg_message("charming_effect",
        "mage unit duration", mage, target, duration));

    return cast_level;
}
