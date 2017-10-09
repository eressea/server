#include <platform.h>
#include "items.h"

#include "alchemy.h"
#include "study.h"
#include "economy.h"
#include "move.h"
#include "magic.h"

#include <attributes/fleechance.h>

#include <spells/shipcurse.h>
#include <spells/unitcurse.h>
#include <spells/regioncurse.h>

#include <kernel/curse.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

/* triggers includes */
#include <triggers/changerace.h>
#include <triggers/timeout.h>

#include <util/attrib.h>
#include <util/event.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/rng.h>

#include <assert.h>
#include <limits.h>

/* BEGIN studypotion */
#define MAXGAIN 15
static int
use_studypotion(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    if (u->thisorder && init_order(u->thisorder, u->faction->locale) == K_STUDY) {
        char token[128];
        skill_t sk = NOSKILL;
        skill *sv = 0;
        const char * s = gettoken(token, sizeof(token));

        if (s) {
            sk = get_skill(s, u->faction->locale);
            sv = unit_skill(u, sk);
        }

        if (sv && sv->level > 2) {
            /* TODO: message */
        }
        else if (sk != NOSKILL && study_cost(u, sk) > 0) {
            /* TODO: message */
        }
        else {
            attrib *a = a_find(u->attribs, &at_learning);
            teaching_info *teach;
            if (a == NULL) {
                a = a_add(&u->attribs, a_new(&at_learning));
            }
            teach = (teaching_info *)a->data.v;
            if (amount > MAXGAIN) {
                amount = MAXGAIN;
            }
            teach->days += amount * STUDYDAYS;
            if (teach->days > MAXGAIN * STUDYDAYS) {
                teach->days = MAXGAIN * STUDYDAYS;
            }
            i_change(&u->items, itype, -amount);
            return 0;
        }
    }
    return EUNUSABLE;
}
/* END studypotion */

/* BEGIN speedsail */
#define SPEEDSAIL_EFFECT 1
static int
use_speedsail(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    double effect;
    ship *sh = u->ship;
    if (!sh) {
        cmistake(u, ord, 20, MSG_MOVE);
        return -1;
    }

    effect = SPEEDSAIL_EFFECT;
    create_curse(u, &sh->attribs, &ct_shipspeedup, 20, INT_MAX, effect, 0);

    ADDMSG(&u->faction->msgs, msg_message("use_speedsail", "unit speed", u,
        SPEEDSAIL_EFFECT));
    use_pooled(u, itype->rtype, GET_DEFAULT, 1);

    return 0;
}

/* END speedsail */

/* ------------------------------------------------------------- */
/* Kann auch von Nichtmagiern benutzt werden, erzeugt eine
* Antimagiezone, die zwei Runden bestehen bleibt */
static int
use_antimagiccrystal(unit * u, const struct item_type *itype, int amount,
struct order *ord)
{
    region *r = u->region;
    const resource_type *rt_crystal = NULL;
    int i;

    rt_crystal = rt_find("antimagic");
    assert(rt_crystal != NULL);

    for (i = 0; i != amount; ++i) {
        int effect, duration = 2;
        double force;
        spell *sp = find_spell("antimagiczone");
        attrib **ap = &r->attribs;
        UNUSED_ARG(ord);
        assert(sp);

        /* Reduziert die St�rke jedes Spruchs um effect */
        effect = 5;

        /* H�lt Spr�che bis zu einem summierten Gesamtlevel von power aus.
         * Jeder Zauber reduziert die 'Lebenskraft' (vigour) der Antimagiezone
         * um seine Stufe */
        force = effect * 20;     /* Stufe 5 =~ 100 */

        /* Regionszauber aufl�sen */
        while (*ap && force > 0) {
            curse *c;
            attrib *a = *ap;
            if (a->type != &at_curse) {
                do {
                    ap = &(*ap)->next;
                } while (*ap && a->type == (*ap)->type);
                continue;
            }
            c = (curse *)a->data.v;

            /* Immunit�t pr�fen */
            if (c_flags(c) & CURSE_IMMUNE) {
                do {
                    ap = &(*ap)->next;
                } while (*ap && a->type == (*ap)->type);
                continue;
            }

            force = destr_curse(c, effect, force);
            if (c->vigour <= 0) {
                a_remove(&r->attribs, a);
            }
            if (*ap)
                ap = &(*ap)->next;
        }

        if (force > 0) {
            create_curse(u, &r->attribs, &ct_antimagiczone, force, duration,
                effect, 0);
        }
    }
    use_pooled(u, rt_crystal, GET_DEFAULT, amount);
    ADDMSG(&u->region->msgs, msg_message("use_antimagiccrystal", "unit", u));
    return 0;
}

#define BAGPIPEFRACTION (dice(2,4)+2)
#define BAGPIPEDURATION (dice(2,10)+4)

static int
use_bagpipeoffear(struct unit *u, const struct item_type *itype,
int amount, struct order *ord)
{
    int money;

    if (get_curse(u->region->attribs, &ct_depression)) {
        cmistake(u, ord, 58, MSG_MAGIC);
        return -1;
    }

    money = entertainmoney(u->region) / BAGPIPEFRACTION;
    change_money(u, money);
    rsetmoney(u->region, rmoney(u->region) - money);

    create_curse(u, &u->region->attribs, &ct_depression,
        20, BAGPIPEDURATION, 0.0, 0);

    ADDMSG(&u->faction->msgs, msg_message("bagpipeoffear_faction",
        "unit region command money", u, u->region, ord, money));

    ADDMSG(&u->region->msgs, msg_message("bagpipeoffear_region",
        "unit money", u, money));

    return 0;
}

static int
use_aurapotion50(struct unit *u, const struct item_type *itype,
int amount, struct order *ord)
{
    if (!is_mage(u)) {
        cmistake(u, ord, 214, MSG_MAGIC);
        return -1;
    }

    change_spellpoints(u, 50);

    ADDMSG(&u->faction->msgs, msg_message("aurapotion50",
        "unit region command", u, u->region, ord));

    use_pooled(u, itype->rtype, GET_DEFAULT, 1);

    return 0;
}

static int
use_birthdayamulet(unit * u, const struct item_type *itype, int amount,
struct order *ord)
{
    direction_t d;
    message *msg = msg_message("meow", "");

    UNUSED_ARG(ord);
    UNUSED_ARG(amount);
    UNUSED_ARG(itype);

    add_message(&u->region->msgs, msg);
    for (d = 0; d < MAXDIRECTIONS; d++) {
        region *tr = rconnect(u->region, d);
        if (tr)
            add_message(&tr->msgs, msg);
    }
    msg_release(msg);
    return 0;
}

static int use_foolpotion(unit *u, const item_type *itype, int amount,
    struct order *ord)
{
    int targetno = read_unitid(u->faction, u->region);
    unit *target = findunit(targetno);
    if (target == NULL || u->region != target->region) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
            ""));
        return ECUSTOM;
    }
    if (effskill(u, SK_STEALTH, 0) <= effskill(target, SK_PERCEPTION, 0)) {
        cmistake(u, ord, 64, MSG_EVENT);
        return ECUSTOM;
    }
    ADDMSG(&u->faction->msgs, msg_message("givedumb",
        "unit recipient amount", u, target, amount));

    change_effect(target, itype->rtype->ptype, amount);
    use_pooled(u, itype->rtype, GET_DEFAULT, amount);
    return 0;
}

static int
use_bloodpotion(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    if (u->number == 0 || u_race(u) == get_race(RC_DAEMON)) {
        change_effect(u, itype->rtype->ptype, 100 * amount);
    }
    else {
        const race *irace = u_irace(u);
        if (irace == u_race(u)) {
            const race *rcfailure = rc_find("smurf");
            if (!rcfailure) {
                rcfailure = rc_find("toad");
            }
            if (rcfailure) {
                trigger *trestore = trigger_changerace(u, u_race(u), irace);
                if (trestore) {
                    int duration = 2 + rng_int() % 8;

                    add_trigger(&u->attribs, "timer", trigger_timeout(duration,
                        trestore));
                    u->irace = NULL;
                    u_setrace(u, rcfailure);
                }
            }
        }
    }
    use_pooled(u, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        amount);
    usetpotionuse(u, itype->rtype->ptype);

    ADDMSG(&u->faction->msgs, msg_message("usepotion",
        "unit potion", u, itype->rtype));
    return 0;
}

static int heal(unit * user, int effect)
{
    int req = unit_max_hp(user) * user->number - user->hp;
    if (req > 0) {
        req = MIN(req, effect);
        effect -= req;
        user->hp += req;
    }
    return effect;
}

static int
use_healingpotion(struct unit *user, const struct item_type *itype, int amount,
struct order *ord)
{
    int effect = amount * 400;
    unit *u = user->region->units;
    effect = heal(user, effect);
    while (effect > 0 && u != NULL) {
        if (u->faction == user->faction) {
            effect = heal(u, effect);
        }
        u = u->next;
    }
    use_pooled(user, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        amount);
    usetpotionuse(user, itype->rtype->ptype);

    ADDMSG(&user->faction->msgs, msg_message("usepotion",
        "unit potion", user, itype->rtype));
    return 0;
}

/* ------------------------------------------------------------- */
/* Kann auch von Nichtmagier benutzt werden, modifiziert Taktik fuer diese
* Runde um -1 - 4 Punkte. */
static int
use_tacticcrystal(unit * u, const struct item_type *itype, int amount,
    struct order *ord)
{
    int i;
    for (i = 0; i != amount; ++i) {
        int duration = 1;           /* wirkt nur in dieser Runde */
        curse *c;
        float effect;
        float power = 5;            /* Widerstand gegen Antimagiesprueche, ist in diesem
                                    Fall egal, da der curse fuer den Kampf gelten soll,
                                    der vor den Antimagiezaubern passiert */

        effect = (float)(rng_int() % 6 - 1);
        c = create_curse(u, &u->attribs, &ct_skillmod, power,
            duration, effect, u->number);
        c->data.i = SK_TACTICS;
        UNUSED_ARG(ord);
    }
    use_pooled(u, itype->rtype, GET_DEFAULT, amount);
    ADDMSG(&u->faction->msgs, msg_message("use_tacticcrystal",
        "unit region", u, u->region));
    return 0;
}

static int
use_mistletoe(struct unit *user, const struct item_type *itype, int amount,
    struct order *ord)
{
    int mtoes =
        get_pooled(user, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
            user->number);

    if (user->number > mtoes) {
        ADDMSG(&user->faction->msgs, msg_message("use_singleperson",
            "unit item region command", user, itype->rtype, user->region, ord));
        return -1;
    }
    use_pooled(user, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        user->number);
    a_add(&user->attribs, make_fleechance((float)1.0));
    ADDMSG(&user->faction->msgs,
        msg_message("use_item", "unit item", user, itype->rtype));

    return 0;
}

static int use_warmthpotion(unit *u, const item_type *itype,
    int amount, struct order *ord)
{
    if (u->faction->race == get_race(RC_INSECT)) {
        u->flags |= UFL_WARMTH;
    }
    else {
        /* nur fuer insekten: */
        cmistake(u, ord, 163, MSG_EVENT);
        return ECUSTOM;
    }
    use_pooled(u, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        amount);
    usetpotionuse(u, itype->rtype->ptype);

    ADDMSG(&u->faction->msgs, msg_message("usepotion",
        "unit potion", u, itype->rtype));
    return 0;
}

void register_itemfunctions(void)
{
    /* have tests: */
    register_item_use(use_mistletoe, "use_mistletoe");
    register_item_use(use_tacticcrystal, "use_dreameye");
    register_item_use(use_studypotion, "use_studypotion");
    register_item_use(use_antimagiccrystal, "use_antimagic");
    register_item_use(use_speedsail, "use_speedsail");
    register_item_use(use_bagpipeoffear, "use_bagpipeoffear");
    register_item_use(use_aurapotion50, "use_aurapotion50");
    register_item_use(use_birthdayamulet, "use_aoc");
    register_item_use(use_foolpotion, "use_p7");
    register_item_use(use_bloodpotion, "use_peasantblood");
    register_item_use(use_healingpotion, "use_ointment");
    register_item_use(use_warmthpotion, "use_nestwarmth");

    /* ungetestet: Wasser des Lebens */
    register_item_use(use_potion_delayed, "use_p2");
}
