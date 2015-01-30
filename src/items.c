#include <platform.h>
#include <kernel/config.h>
#include "items.h"

#include "study.h"
#include "move.h"
#include "magic.h"

#include <kernel/curse.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

#include <items/demonseye.h>

#include <util/attrib.h>
#include <util/parser.h>
#include <util/rand.h>

#include <assert.h>
#include <limits.h>

/* BEGIN studypotion */
#define MAXGAIN 15
static int
use_studypotion(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    if (init_order(u->thisorder) == K_STUDY) {
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
            teach->value += amount * 30;
            if (teach->value > MAXGAIN * 30) {
                teach->value = MAXGAIN * 30;
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
    curse *c;
    float effect;
    ship *sh = u->ship;
    if (!sh) {
        cmistake(u, ord, 20, MSG_MOVE);
        return -1;
    }

    effect = SPEEDSAIL_EFFECT;
    c =
        create_curse(u, &sh->attribs, ct_find("shipspeedup"), 20, INT_MAX, effect,
        0);
    c_setflag(c, CURSE_NOAGE);

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

    if (rt_crystal == NULL) {
        rt_crystal = rt_find("antimagic");
        assert(rt_crystal != NULL);
    }
    for (i = 0; i != amount; ++i) {
        int effect, duration = 2;
        float force;
        spell *sp = find_spell("antimagiczone");
        attrib **ap = &r->attribs;
        unused_arg(ord);
        assert(sp);

        /* Reduziert die Stärke jedes Spruchs um effect */
        effect = 5;

        /* Hält Sprüche bis zu einem summierten Gesamtlevel von power aus.
         * Jeder Zauber reduziert die 'Lebenskraft' (vigour) der Antimagiezone
         * um seine Stufe */
        force = (float)effect * 20;     /* Stufe 5 =~ 100 */

        /* Regionszauber auflösen */
        while (*ap && force > 0) {
            curse *c;
            attrib *a = *ap;
            if (!fval(a->type, ATF_CURSE)) {
                do {
                    ap = &(*ap)->next;
                } while (*ap && a->type == (*ap)->type);
                continue;
            }
            c = (curse *)a->data.v;

            /* Immunität prüfen */
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
            create_curse(u, &r->attribs, ct_find("antimagiczone"), (float)force, duration,
                (float)effect, 0);
        }
    }
    use_pooled(u, rt_crystal, GET_DEFAULT, amount);
    ADDMSG(&u->faction->msgs, msg_message("use_antimagiccrystal",
        "unit region", u, r));
    return 0;
}

static int
use_instantartsculpture(struct unit *u, const struct item_type *itype,
int amount, struct order *ord)
{
    building *b;

    if (u->region->land == NULL) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_onlandonly", ""));
        return -1;
    }

    b = new_building(bt_find("artsculpture"), u->region, u->faction->locale);
    b->size = 100;

    ADDMSG(&u->region->msgs, msg_message("artsculpture_create", "unit region",
        u, u->region));

    use_pooled(u, itype->rtype, GET_DEFAULT, 1);

    return 0;
}

static int
use_instantartacademy(struct unit *u, const struct item_type *itype,
int amount, struct order *ord)
{
    building *b;

    if (u->region->land == NULL) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_onlandonly", ""));
        return -1;
    }

    b = new_building(bt_find("artacademy"), u->region, u->faction->locale);
    b->size = 100;

    ADDMSG(&u->region->msgs, msg_message("artacademy_create", "unit region", u,
        u->region));

    use_pooled(u, itype->rtype, GET_DEFAULT, 1);

    return 0;
}

#define BAGPIPEFRACTION dice_rand("2d4+2")
#define BAGPIPEDURATION dice_rand("2d10+4")

static int
use_bagpipeoffear(struct unit *u, const struct item_type *itype,
int amount, struct order *ord)
{
    int money;

    if (get_curse(u->region->attribs, ct_find("depression"))) {
        cmistake(u, ord, 58, MSG_MAGIC);
        return -1;
    }

    money = entertainmoney(u->region) / BAGPIPEFRACTION;
    change_money(u, money);
    rsetmoney(u->region, rmoney(u->region) - money);

    create_curse(u, &u->region->attribs, ct_find("depression"),
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

void register_itemfunctions(void)
{
    register_demonseye();
    register_item_use(use_antimagiccrystal, "use_antimagiccrystal");
    register_item_use(use_instantartsculpture, "use_instantartsculpture");
    register_item_use(use_studypotion, "use_studypotion");
    register_item_use(use_speedsail, "use_speedsail");
    register_item_use(use_instantartacademy, "use_instantartacademy");
    register_item_use(use_bagpipeoffear, "use_bagpipeoffear");
    register_item_use(use_aurapotion50, "use_aurapotion50");
}
