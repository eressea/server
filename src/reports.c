#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "reports.h"

#include "battle.h"
#include "defaults.h"
#include "guard.h"
#include "laws.h"
#include "spells.h"
#include "teleport.h"
#include "travelthru.h"
#include "lighthouse.h"
#include "donations.h"

/* attributes includes */
#include "attributes/attributes.h"
#include "attributes/follow.h"
#include "attributes/otherfaction.h"
#include "attributes/racename.h"
#include "attributes/stealth.h"

#include "spells/regioncurse.h"
#include "spells/unitcurse.h"

/* kernel includes */
#include "kernel/config.h"
#include "kernel/calendar.h"
#include "kernel/curse.h"
#include "kernel/ally.h"
#include "kernel/alliance.h"
#include "kernel/connection.h"
#include "kernel/building.h"
#include "kernel/curse.h"
#include "kernel/faction.h"
#include "kernel/group.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/plane.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/resources.h"
#include "kernel/ship.h"
#include "kernel/skills.h"
#include "kernel/spell.h"
#include "kernel/spellbook.h"
#include "kernel/terrain.h"
#include "kernel/unit.h"

/* util includes */
#include "kernel/attrib.h"
#include "util/base36.h"
#include "util/functions.h"
#include "util/goodies.h"
#include "util/language.h"
#include "util/lists.h"
#include "util/log.h"
#include "util/macros.h"
#include "util/message.h"
#include "util/path.h"
#include "util/password.h"
#include "util/translation.h"

#include <selist.h>
#include <stream.h>
#include <strings.h>

#include <gb_string.h>
#include <stb_ds.h>

/* libc includes */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "move.h"

#if defined(_MSC_VER) && _MSC_VER >= 1900
# pragma warning(disable: 4774) /* TODO: remove this */
#endif

#define SCALEWEIGHT      100    /* Faktor, um den die Anzeige von Gewichten skaliert wird */

bool nocr = false;
bool nonr = false;
bool noreports = false;

const char *visibility[] = {
    "none",
    "neighbour",
    "lighthouse",
    "lighthouse",
    "travel",
    "far",
    "unit",
    "spell",
    "battle"
};

const char *coasts[MAXDIRECTIONS] = {
    "coast::nw",
    "coast::ne",
    "coast::e",
    "coast::se",
    "coast::sw",
    "coast::w"
};

const char *options[MAXOPTIONS] = {
    "AUSWERTUNG",
    "COMPUTER",
    "ZUGVORLAGE",
    NULL,
    "STATISTIK",
    "DEBUG",
    "ZIPPED",
    "ZEITUNG",                    /* Option hat Sonderbehandlung! */
    NULL,
    "ADRESSEN",
    "BZIP2",
    "PUNKTE",
    "SHOWSKCHANGE"
};

bool omniscient(const faction *f)
{
    static const race *rc_template;
    static int cache;
    if (rc_changed(&cache)) {
        rc_template = get_race(RC_TEMPLATE);
    }
    return (f->race == rc_template);
}

static char *groupid(const struct group *g, const struct faction *f)
{
    typedef char name[OBJECTIDSIZE + 1];
    static name idbuf[8];
    static int nextbuf = 0;
    char *buf = idbuf[(++nextbuf) % 8];
    sprintf(buf, "%s (%s)", g->name, itoa36(f->no));
    return buf;
}

const char *combatstatus[] = {
    "status_aggressive", "status_front",
    "status_rear", "status_defensive",
    "status_avoid", "status_flee"
};

void report_status(const unit * u, const struct locale *lang, struct sbstring *sbp)
{
    const char * status = LOC(lang, combatstatus[u->status]);

    if (!status) {
        const char *lname = locale_name(lang);
        struct locale *wloc = get_locale(lname);

        log_warning("no translation for combat status %s in %s", combatstatus[u->status], lname);
        locale_setstring(wloc, combatstatus[u->status], combatstatus[u->status] + 7);
        sbs_strcat(sbp, combatstatus[u->status] + 7);
    }
    else {
        sbs_strcat(sbp, status);
    }
    if (fval(u, UFL_NOAID)) {
        sbs_strcat(sbp, ", ");
        sbs_strcat(sbp, LOC(lang, "status_noaid"));
    }
}

const char *hp_status(const unit * u)
{
    double p;
    int max_hp = u->number * unit_max_hp(u);

    if (u->hp == max_hp)
        return NULL;

    p = (double)((double)u->hp / (double)(max_hp));

    if (p < 0.50)
        return mkname("damage", "badly");
    if (p < 0.75)
        return mkname("damage", "wounded");
    if (p < 0.99)
        return mkname("damage", "exhausted");
    if (p > 2.00)
        return mkname("damage", "plusstrong");
    if (p > 1.50)
        return mkname("damage", "strong");

    return NULL;
}

bool see_schemes(const region *r, seen_mode mode) {
    return mode >= seen_unit && is_astral(r) && !is_cursed(r->attribs, &ct_astralblock);
}

void
report_item(const unit * owner, const item * i, const faction * viewer,
    const char **name, const char **basename, int *number, bool singular)
{
    const resource_type *rsilver = get_resourcetype(R_SILVER);

    assert(!owner || owner->number);
    if (owner && owner->faction == viewer) {
        if (name)
            *name =
            LOC(viewer->locale, resourcename(i->type->rtype,
            ((i->number != 1 && !singular) ? GR_PLURAL : 0)));
        if (basename)
            *basename = resourcename(i->type->rtype, 0);
        if (number)
            *number = i->number;
    }
    else if (owner && i->type->rtype == rsilver) {
        int pp = i->number / owner->number;
        if (number)
            *number = 1;
        if (pp > 50000 && dragonrace(u_race(owner))) {
            if (name)
                *name = LOC(viewer->locale, "dragonhoard");
            if (basename)
                *basename = "dragonhoard";
        }
        else if (pp > 5000) {
            if (name)
                *name = LOC(viewer->locale, "moneychest");
            if (basename)
                *basename = "moneychest";
        }
        else if (pp > 500) {
            if (name)
                *name = LOC(viewer->locale, "moneybag");
            if (basename)
                *basename = "moneybag";
        }
        else {
            if (number)
                *number = 0;
            if (name)
                *name = NULL;
            if (basename)
                *basename = NULL;
        }
    }
    else {
        if (name)
            *name =
            LOC(viewer->locale, resourcename(i->type->rtype,
                NMF_APPEARANCE | ((i->number != 1 && !singular) ? GR_PLURAL : 0)));
        if (basename)
            *basename = resourcename(i->type->rtype, NMF_APPEARANCE);
        if (number) {
            if (fval(i->type, ITF_HERB))
                *number = 1;
            else
                *number = i->number;
        }
    }
}

#define ORDERS_IN_NR 1
static void buforder(sbstring *sbp, const order * ord, const struct locale *lang, int mode)
{
    sbs_strcat(sbp, ", \"");
    if (mode < ORDERS_IN_NR) {
        char cmd[ORDERSIZE];
        get_command(ord, lang, cmd, sizeof(cmd));
        sbs_strcat(sbp, cmd);
    }
    else {
        sbs_strcat(sbp, "...");
    }

    sbs_strcat(sbp, "\"");
}

/** create a report of a list of items to a non-owner.
 * \param result: an array of size items.
 * \param size: maximum number of items to return
 * \param owner: the owner of the items, or NULL for faction::items etc.
 * \param viewer: the faction looking at the items
 */
int
report_items(const unit *u, item * result, int size, const unit * owner,
    const faction * viewer)
{
    const item *itm, *items = u->items;
    int n = 0;                    /* number of results */

    assert(owner == NULL || viewer != owner->faction);
    assert(size);

    if (u->attribs) {
        curse * cu = get_curse(u->attribs, &ct_itemcloak);
        if (cu && curse_active(cu)) {
            return 0;
        }
    }
    for (itm = items; itm; itm = itm->next) {
        const char *ic;

        report_item(owner, itm, viewer, NULL, &ic, NULL, false);
        if (ic && *ic) {
            item *ishow;
            for (ishow = result; ishow != result + n; ++ishow) {
                const char *sc;

                if (ishow->type == itm->type)
                    sc = ic;
                else
                    report_item(owner, ishow, viewer, NULL, &sc, NULL, false);
                if (sc == ic || strcmp(sc, ic) == 0) {
                    ishow->number += itm->number;
                    break;
                }
            }
            if (ishow == result + n) {
                if (n == size) {
                    log_error("too many items to report, increase buffer size.\n");
                    return -1;
                }
                result[n].number = itm->number;
                result[n].type = itm->type;
                result[n].next = (n + 1 == size) ? NULL : result + n + 1;
                ++n;
            }
        }
    }
    if (n > 0)
        result[n - 1].next = NULL;
    return n;
}

static void report_resource(resource_report * result, const resource_type *rtype,
    int number, int level)
{
    assert(rtype);
    result->rtype = rtype;
    result->number = number;
    result->level = level;
}

static void bufattack(struct sbstring *sbp, const struct locale *lang, const char *name, const char *dmg) {
    sbs_strcat(sbp, LOC(lang, name));
    if (dmg) {
        sbs_strcat(sbp, " (");
        sbs_strcat(sbp, dmg);
        sbs_strcat(sbp, ")");
    }
}

void report_raceinfo(const struct race *rc, const struct locale *lang, struct sbstring *sbp)
{
    const char *info;
    int a, at_count;
    const char *name, *key;

    name = rc_name_s(rc, NAME_SINGULAR);
    sbs_strcat(sbp, LOC(lang, name));
    sbs_strcat(sbp, ": ");

    key = mkname("raceinfo", rc->_name);
    info = locale_getstring(lang, key);
    if (info == NULL) {
        info = LOC(lang, mkname("raceinfo", "no_info"));
    }

    if (info) {
        sbs_strcat(sbp, info);
    }

    /* hp_p : Trefferpunkte */
    sbs_strcat(sbp, " ");
    sbs_strcat(sbp, str_itoa(rc->hitpoints));
    sbs_strcat(sbp, " ");
    sbs_strcat(sbp, LOC(lang, "stat_hitpoints"));

    /* b_attacke : Angriff */
    sbs_strcat(sbp, ", ");
    sbs_strcat(sbp, LOC(lang, "stat_attack"));
    sbs_strcat(sbp, ": ");
    sbs_strcat(sbp, str_itoa(rc->at_default + rc->at_bonus));

    /* b_defense : Verteidigung */
    sbs_strcat(sbp, ", ");
    sbs_strcat(sbp, LOC(lang, "stat_defense"));
    sbs_strcat(sbp, ": ");
    sbs_strcat(sbp, str_itoa(rc->df_default + rc->df_bonus));

    /* b_armor : Ruestung */
    if (rc->armor > 0) {
        sbs_strcat(sbp, ", ");
        sbs_strcat(sbp, LOC(lang, "stat_armor"));
        sbs_strcat(sbp, ": ");
        sbs_strcat(sbp, str_itoa(rc->armor));
    }

    sbs_strcat(sbp, ".");

    /* b_damage : Schaden */
    at_count = 0;
    for (a = 0; a < RACE_ATTACKS; a++) {
        if (rc->attack[a].type != AT_NONE) {
            at_count++;
        }
    }
    if (rc->battle_flags & BF_EQUIPMENT) {
        sbs_strcat(sbp, " ");
        sbs_strcat(sbp, LOC(lang, "stat_equipment"));
    }
    if (rc->battle_flags & BF_RES_PIERCE) {
        sbs_strcat(sbp, " ");
        sbs_strcat(sbp, LOC(lang, "stat_pierce"));
    }
    if (rc->battle_flags & BF_RES_CUT) {
        sbs_strcat(sbp, " ");
        sbs_strcat(sbp, LOC(lang, "stat_cut"));
    }
    if (rc->battle_flags & BF_RES_BASH) {
        sbs_strcat(sbp, " ");
        sbs_strcat(sbp, LOC(lang, "stat_bash"));
    }

    sbs_strcat(sbp, " ");
    sbs_strcat(sbp, str_itoa(at_count));
    sbs_strcat(sbp, " ");
    sbs_strcat(sbp, LOC(lang, (at_count == 1) ? "stat_attack" : "stat_attacks"));

    for (a = 0; a < RACE_ATTACKS; a++) {
        if (rc->attack[a].type != AT_NONE) {
            sbs_strcat(sbp, (a == 0) ? ": " : ", ");

            switch (rc->attack[a].type) {
            case AT_STANDARD:
                bufattack(sbp, lang, "attack_standard", rc->def_damage);
                break;
            case AT_NATURAL:
                bufattack(sbp, lang, "attack_natural", rc->attack[a].data.dice);
                break;
            case AT_SPELL:
            case AT_COMBATSPELL:
            case AT_DRAIN_ST:
            case AT_DRAIN_EXP:
            case AT_DAZZLE:
                bufattack(sbp, lang, "attack_magical", NULL);
                break;
            case AT_STRUCTURAL:
                bufattack(sbp, lang, "attack_structural", rc->attack[a].data.dice);
                break;
            }
        }
    }

    sbs_strcat(sbp, ".");
}

void
report_building(const struct building *b, const char **name,
    const char **illusion)
{
    if (name) {
        *name = buildingtype(b->type, b, b->size);
    }
    if (illusion) {
        const building_type *btype = visible_building(b);
        if (btype != b->type) {
            *illusion = buildingtype(btype, b, b->size);
        }
        else {
            *illusion = NULL;
        }
    }
}

int
report_resources(const region * r, resource_report result[MAX_RAWMATERIALS],
    const faction * viewer, enum seen_mode mode)
{
    int n = 0;
    const resource_type* rtype;

    if (r->land) {
        int peasants = rpeasants(r);
        if (peasants) {
            rtype = get_resourcetype(R_PEASANT);
            if (rtype) {
                report_resource(result + n, rtype, peasants, -1);
                ++n;
            }
        }

        if (mode >= seen_lighthouse_land) {
            int trees = rtrees(r, 2);
            int saplings = rtrees(r, 1);
            bool mallorn = fval(r, RF_MALLORN) != 0;
            if (trees) {
                rtype = get_resourcetype(mallorn ? R_MALLORN_TREE : R_TREE);
                if (rtype) {
                    report_resource(result + n, rtype, trees, -1);
                    ++n;
                }
            }
            if (saplings) {
                rtype = get_resourcetype(mallorn ? R_MALLORN_SAPLING : R_SAPLING);
                if (rtype) {
                    report_resource(result + n, rtype, saplings, -1);
                    ++n;
                }
            }
        }
    }
    if (mode >= seen_travel) {
        ptrdiff_t i, len = arrlen(r->resources);
        int money = rmoney(r);
        int horses = rhorses(r);
        if (money) {
            rtype = get_resourcetype(R_SILVER);
            if (rtype) {
                report_resource(result + n, rtype, money, -1);
                ++n;
            }
        }
        if (horses) {
            rtype = get_resourcetype(R_HORSE);
            if (rtype) {
                report_resource(result + n, rtype, horses, -1);
                ++n;
            }
        }
        for (i = 0; i != len ; ++i) {
            rawmaterial* res = r->resources + i;
            const item_type *itype = resource2item(res->rtype);
            int minskill = itype->construction->minskill;
            skill_t skill = itype->construction->skill;
            int level = res->level + minskill - 1;
            int visible = -1;
            rawmaterial_type *raw = rmt_get(res->rtype);
            if (raw->visible == NULL) {
                visible = res->amount;
                level = res->level + minskill - 1;
            }
            else {
                const unit *u;
                int maxskill = 0;
                for (u = r->units; visible != res->amount && u != NULL; u = u->next) {
                    if (u->faction == viewer) {
                        int s = effskill(u, skill, NULL);
                        if (s > maxskill) {
                            maxskill = s;
                            visible = raw->visible(res, maxskill);
                        }
                    }
                }
            }
            if (level >= 0 && visible >= 0) {
                if (n >= MAX_RAWMATERIALS) {
                    log_error("More than MAX_RAWMATERIALS resources in %s", regionname(r, NULL));
                }
                else {
                    report_resource(result + n, res->rtype, visible, level);
                    ++n;
                }
            }
        }
    }
    return n;
}

static void spskill(struct skill* sv, const struct unit *u, const struct locale * lang, struct sbstring* sbp)
{
    int effsk;

    sbs_strcat(sbp, skillname(sv->id, lang));
    sbs_strcat(sbp, " ");

    if (sv->id == SK_MAGIC) {
        magic_t mtype = unit_get_magic(u);
        if (mtype != M_NONE && mtype != M_GRAY && mtype < MAXMAGIETYP) {
            sbs_strcat(sbp, magic_name(mtype, lang));
            sbs_strcat(sbp, " ");
        }
    }

    if (sv->id == SK_STEALTH && fval(u, UFL_STEALTH)) {
        int i = u_geteffstealth(u);
        if (i >= 0) {
            sbs_strcat(sbp, str_itoa(i));
            sbs_strcat(sbp, "/");
        }
    }

    effsk = eff_skill(u, sv, NULL);
    sbs_strcat(sbp, str_itoa(effsk));

    if (u->faction->options & WANT_OPTION(O_SHOWSKCHANGE)) {
        int oldeff = 0;
        int diff;

        if (sv->old > 0) {
            oldeff = sv->old + get_modifier(u, sv->id, sv->old, u->region, false);
            if (oldeff < 0) oldeff = 0;
        }

        diff = effsk - oldeff;

        if (diff != 0) {
            sbs_strcat(sbp, " (");
            sbs_strcat(sbp, (diff > 0) ? "+" : "");
            sbs_strcat(sbp, str_itoa(diff));
            sbs_strcat(sbp, ")");
        }
    }
}

static void bufunit_info(const faction* f, const unit* u, const faction* of,
    bool anon, struct sbstring* sbp)
{
    const struct locale* lang = f->locale;
    const group *g = NULL;
    const faction* df = NULL;

    if (u->faction == f) {
        if (fval(u, UFL_GROUP)) {
            g = get_group(u);
        }
    }
    else if (anon || of) {
        if (alliedunit(u, f, HELP_FSTEALTH)) {
            df = u->faction;
        }
        else {
            if (of != f) {
                /* cannot see through the disguise */
                if (anon) {
                    df = of = NULL;
                }
                else {
                    df = of;
                    of = NULL;
                }
            }
        }
        if (anon) {
            of = NULL;
        }
    }
    else {
        df = u->faction;
    }
    if (g) {
        sbs_strcat(sbp, ", ");
        sbs_strcat(sbp, groupid(g, f));
    }
    else if (df) {
        sbs_strcat(sbp, ", ");
        sbs_strcat(sbp, factionname(df));
    }

    if (of) {
        sbs_strcat(sbp, ", ");
        sbs_strcat(sbp, LOC(lang, "disguised_as"));
        sbs_strcat(sbp, " ");
        sbs_strcat(sbp, factionname(of));
    }
    if (anon) {
        sbs_strcat(sbp, ", ");
        sbs_strcat(sbp, LOC(lang, "anonymous"));
    }
}

static void bufunit_race(const faction* f, const unit* u, struct sbstring* sbp)
{
    const struct locale* lang = f->locale;
    const char *str = get_racename(u->attribs);
    if (str) {
        char buffer[64];
        const char* name;
        if (u->number != 1) {
            snprintf(buffer, sizeof(buffer), "%s_p", str);
            str = buffer;
        }
        name = locale_string(lang, mkname("race", str), false);
        sbs_strcat(sbp, name ? name : str);
        if (u->faction == f) {
            sbs_strcat(sbp, " (");
            sbs_strcat(sbp, racename(lang, u, u_race(u)));
            sbs_strcat(sbp, ")");
        }
    }
    else {
        const race* irace = u_irace(u);
        const race* urace = u_race(u);
        sbs_strcat(sbp, racename(lang, u, irace));
        if (u->faction == f && irace != urace) {
            sbs_strcat(sbp, " (");
            sbs_strcat(sbp, racename(lang, u, urace));
            sbs_strcat(sbp, ")");
        }
    }
}

static void bufunit_skills(const unit* u, const struct locale* lang, struct sbstring* sbp)
{
    int dh = 0;
    size_t s, len;
    for (len = arrlen(u->skills), s = 0; s != len; ++s) {
        skill* sv = u->skills + s;
        if (!u->number) {
            continue;
        }
        if (sv->level <= 0) {
            if (sv->old <= 0 || (u->faction->options & WANT_OPTION(O_SHOWSKCHANGE)) == 0) {
                continue;
            }
        }
        sbs_strcat(sbp, ", ");

        if (!dh) {
            sbs_strcat(sbp, LOC(lang, "nr_skills"));
            sbs_strcat(sbp, ": ");
            dh = 1;
        }
        spskill(sv, u, lang, sbp);
    }
}

static void bufunit_items(const item* items, const unit *u, const faction *f, struct sbstring* sbp)
{
    const struct locale* lang = f->locale;
    int dh = 0;
    const item* itm;
    for (itm = items; itm; itm = itm->next) {
        const char* ic;
        int in;
        report_item(u, itm, f, &ic, NULL, &in, false);
        if (in == 0 || ic == NULL) {
            continue;
        }
        sbs_strcat(sbp, ", ");

        if (!dh) {
            sbs_strcat(sbp, LOC(lang, "nr_inventory"));
            sbs_strcat(sbp, ": ");
            dh = 1;
        }
        if (in == 1) {
            sbs_strcat(sbp, ic);
        }
        else {
            sbs_strcat(sbp, str_itoa(in));
            sbs_strcat(sbp, " ");
            sbs_strcat(sbp, ic);
        }
    }
}

static void bufunit_orders(const unit* u, struct sbstring* sbp)
{
    const struct locale* lang = u->faction->locale;
    int printed = 0;
    const order* ord;
    for (ord = u->orders; ord; ord = ord->next) {
        keyword_t kwd = getkeyword(ord);
        if (is_repeated(kwd)) {
            buforder(sbp, ord, lang, printed++);
            if (printed >= ORDERS_IN_NR) {
                break;
            }
        }
    }
}

static void bufunit_description(const unit* u, const struct faction* f, struct sbstring* sbp)
{
    const struct locale* lang = f->locale;
    const char *str = u_description(u, lang);
    int i = 0;

    if (str) {
        sbs_strcat(sbp, "; ");
        sbs_strcat(sbp, str);
        i = str[strlen(str) - 1];
    }
    if (i != '!' && i != '?' && i != '.') {
        sbs_strcat(sbp, ".");
    }
    str = uprivate(u);
    if (u->faction == f && str) {
        sbs_strcat(sbp, " (Bem: ");
        sbs_strcat(sbp, str);
        sbs_strcat(sbp, ")");
    }
}

static void bufunit_spells(const unit* u, struct sbstring* sbp)
{
    const struct locale* lang = u->faction->locale;
    spellbook* book = unit_get_spellbook(u);

    if (book) {
        selist* ql = book->spells;
        int i, qi, header, maxlevel = effskill(u, SK_MAGIC, NULL);
        sbs_printf(sbp, ". Aura %d/%d", get_spellpoints(u), max_spellpoints(u, NULL));

        for (header = 0, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
            spellbook_entry* sbe = (spellbook_entry*)selist_get(ql, qi);
            const spell* sp = spellref_get(&sbe->spref);
            if (sbe->level <= maxlevel) {
                if (!header) {
                    sbs_printf(sbp, ", %s: ", LOC(lang, "nr_spells"));
                    header = 1;
                }
                else {
                    sbs_strcat(sbp, ", ");
                }
                /* TODO: no need to deref the spellref here (spref->name is good) */
                sbs_strcat(sbp, spell_name(mkname_spell(sp), lang));
            }
        }

        for (i = 0; i != MAXCOMBATSPELLS; ++i) {
            if (get_combatspell(u, i))
                break;
        }
        if (i != MAXCOMBATSPELLS) {
            int dh = 0;
            sbs_printf(sbp, ", %s: ", LOC(lang, "nr_combatspells"));
            for (i = 0; i < MAXCOMBATSPELLS; i++) {
                const spell* sp;
                if (!dh) {
                    dh = 1;
                }
                else {
                    sbs_strcat(sbp, ", ");
                }
                sp = get_combatspell(u, i);
                if (sp) {
                    int sl = get_combatspelllevel(u, i);
                    sbs_strcat(sbp, spell_name(mkname_spell(sp), lang));
                    if (sl > 0) {
                        sbs_printf(sbp, "(%d)", sl);
                    }
                }
                else {
                    sbs_strcat(sbp, LOC(lang, "nr_nospells"));
                }
            }
        }
    }
}

static void bufunit_status(const unit* u, const struct locale *lang, struct sbstring* sbp)
{
    const char* c = hp_status(u);
    c = c ? LOC(lang, c) : 0;
    sbs_strcat(sbp, ", ");
    report_status(u, lang, sbp);
    if (c || fval(u, UFL_HUNGER)) {
        sbs_strcat(sbp, " (");
        if (c) {
            sbs_strcat(sbp, c);
        }
        if (fval(u, UFL_HUNGER)) {
            if (c) {
                sbs_strcat(sbp, ", ");
            }
            sbs_strcat(sbp, LOC(lang, "unit_hungers"));
        }
        sbs_strcat(sbp, ")");
    }
}

void bufunit(const faction * f, const unit * u, const faction * of,
    enum seen_mode mode, bool anon, struct sbstring *sbp)
{
    bool isbattle = (mode == seen_battle);
    item *show = NULL;
    item results[MAX_INVENTORY];
    const struct locale *lang = f->locale;

    assert(f);
    sbs_strcat(sbp, unitname(u));
    if (!isbattle) {
        bufunit_info(f, u, of, anon, sbp);
    }
    sbs_strcat(sbp, ", ");
    sbs_strcat(sbp, str_itoa(u->number));
    sbs_strcat(sbp, " ");

    bufunit_race(f, u, sbp);
    if (fval(u, UFL_HERO) && (u->faction == f || omniscient(f))) {
        sbs_strcat(sbp, ", ");
        sbs_strcat(sbp, LOC(lang, "hero"));
    }
    /* status */

    if (u->number && (u->faction == f || isbattle)) {
        bufunit_status(u, lang, sbp);
    }
    if (is_guard(u)) {
        sbs_strcat(sbp, ", ");
        sbs_strcat(sbp, LOC(lang, "unit_guards"));
    }

    if (u->faction == f) {
        bufunit_skills(u, lang, sbp);
    }

    if (f == u->faction || omniscient(f)) {
        show = u->items;
    }
    else if (mode >= seen_unit) {
        int n = report_items(u, results, MAX_INVENTORY, u, f);
        assert(n >= 0);
        if (n > 0) {
            show = results;
        }
    }
    bufunit_items(show, u, f, sbp);

    if (u->faction == f) {
        bufunit_spells(u, sbp);
        if (!isbattle) {
            bufunit_orders(u, sbp);
        }
    }
    bufunit_description(u, f, sbp);
}

char bufunit_bullet(const faction* f, const unit* u, faction* of, bool anon)
{
    char bullet = '-';
    if (u->faction == f) {
        bullet = '*';
    }
    else if (is_allied(u->faction, f)) {
        bullet = 'o';
    }
    else if (u->attribs && f != u->faction
        && !anon && of == f) {
        bullet = '!';
    }
    else {
        const faction* vf = visible_faction(f, u, of);
        if (vf && !anon && alliedfaction(f, vf, HELP_ALL)) {
            bullet = '+';
        }
    }
    return bullet;
}

void bufunit_depr(const faction * f, const unit * u, enum seen_mode mode,
    char *buf, size_t size)
{
    bool anon = (0 != fval(u, UFL_ANON_FACTION));
    faction * of = get_otherfaction(u);
    sbstring sbs;

    sbs_init(&sbs, buf, size);
    bufunit(f, u, of, mode, anon, &sbs);
}

void split_paragraph(strlist ** SP, const char *s, unsigned int indent, unsigned int width, char mark)
{
    bool firstline;
    static char buf[REPORTWIDTH + 1]; /* FIXME: static buffer, artificial limit */
    size_t len = strlen(s);

    assert(width <= REPORTWIDTH);
    width -= indent;
    firstline = (mark != 0 && indent > 2);
    *SP = 0;

    while (len > 0) {
        unsigned int j;
        const char *cut = NULL, *space = strchr(s, ' ');
        while (space && *space && (space - s) <= (ptrdiff_t)width) {
            cut = space;
            space = strchr(space + 1, ' ');
            if (!space && len < width) {
                cut = space = s + len;
            }
        }

        for (j = 0; j != indent; j++)
            buf[j] = ' ';

        if (firstline) {
            buf[indent - 2] = mark;
            firstline = false;
        }
        if (!cut) {
            cut = s + ((len < REPORTWIDTH) ? len : REPORTWIDTH);
        }
        memcpy(buf + indent, s, cut - s);
        buf[indent + (cut - s)] = 0;
        addstrlist(SP, str_strdup(buf)); /* TODO: too much string copying, cut out this function */
        while (*cut == ' ') {
            ++cut;
        }
        len -= (cut - s);
        s = cut;
    }
}

void sparagraph(strlist ** SP, const char *s, unsigned int indent, char mark)
{
    split_paragraph(SP, s, indent, REPORTWIDTH, mark);
}

struct message *msg_curse(const struct curse *c, const void *obj, objtype_t typ,
    int self)
{
    if (c->type->curseinfo) {
        /* if curseinfo returns NULL, then we don't want to tell the viewer anything. */
        return c->type->curseinfo(obj, typ, c, self);
    }
    else {
        message *msg = cinfo_simple(obj, typ, c, self);
        if (msg == NULL) {
            const char *unknown[] =
            { "unit_unknown", "region_unknown", "building_unknown",
            "ship_unknown" };
            msg = msg_message(mkname("curseinfo", unknown[typ]), "id", c->no);
            log_warning("no curseinfo function for %s and no fallback either.\n", c->type->cname);
        }
        else {
            log_debug("no curseinfo function for %s, using cinfo_simple fallback.\n", c->type->cname);
        }
        return msg;
    }
}

int stealth_modifier(const region *r, const faction *f, enum seen_mode mode)
{
    switch (mode) {
    case seen_spell:
        return get_observer(r, f);
    case seen_unit:
        return 0;
    case seen_lighthouse:
    case seen_travel:
        return -1;
    default:
        return INT_MIN;
    }
}

static void transfer_seen(selist ** dst, selist ** src) {
    assert(!*dst);
    *dst = *src;
    *src = NULL;
}

int cmp_faction(const void *lhs, const void *rhs) {
    const faction *lhf = (const faction *)lhs;
    const faction *rhf = (const faction *)rhs;
    if (lhf->no == rhf->no) return 0;
    if (lhf->no > rhf->no) return 1;
    return -1;
}

static void add_seen_faction_i(struct selist **flist, faction *f) {
    if (f) {
        selist_set_insert(flist, f, cmp_faction);
    }
}

void add_seen_faction(faction *self, faction *seen) {
    add_seen_faction_i(&self->seen_factions, seen);
}

typedef struct address_data {
    faction *f, *lastf;
    selist **flist;
    int stealthmod;
} address_data;

static void cb_add_address(region *r, const unit *ut, void *cbdata) {
    address_data *data = (address_data *)cbdata;
    faction *f = data->f;

    if (ut->faction == f) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            faction *sf = visible_faction(f, u, get_otherfaction(u));
            assert(u->faction != f);   /* if this is see_travel only, then I shouldn't be here. */
            if (data->lastf != sf && cansee_unit(ut, r, u, data->stealthmod)) {
                add_seen_faction_i(data->flist, sf);
                data->lastf = sf;
            }
        }
    }
}

static void add_travelthru_addresses(region *r, faction *f, selist **flist, int stealthmod) {
    /* for each traveling unit: add the faction of any unit is can see */
    address_data cbdata = { 0 };
    cbdata.f = f;
    cbdata.flist = flist;
    cbdata.stealthmod = stealthmod;
    travelthru_map(r, cb_add_address, &cbdata);
}

void get_addresses(report_context * ctx)
{
    /* "TODO: travelthru" */
    region *r;
    const faction *lastf = NULL;
    selist *flist = 0;

    transfer_seen(&flist, &ctx->f->seen_factions);

    ctx->f->seen_factions = NULL; /* do not delete it twice */
    selist_push(&flist, ctx->f);

    if (f_get_alliance(ctx->f)) {
        selist *ql = ctx->f->alliance->members;
        int qi;
        for (qi = 0; ql; selist_advance(&ql, &qi, 1)) {
            add_seen_faction_i(&flist, (faction *)selist_get(ql, qi));
        }
    }

    /* find the first region that this faction can see */
    for (r = ctx->first; r != ctx->last; r = r->next) {
        if (r->seen.mode > seen_none) break;
    }

    for (; r != NULL; r = r->next) {
        if (r->seen.mode >= seen_lighthouse) {
            int stealthmod = stealth_modifier(r, ctx->f, r->seen.mode);
            if (r->seen.mode == seen_lighthouse) {
                unit *u = r->units;
                for (; u; u = u->next) {
                    faction *sf = visible_faction(ctx->f, u, get_otherfaction(u));
                    if (lastf != sf) {
                        if (u->building || u->ship || (stealthmod > INT_MIN
                            && visible_unit(u, ctx->f, stealthmod, seen_lighthouse)))
                        {
                            add_seen_faction_i(&flist, sf);
                            lastf = sf;
                        }
                    }
                }
            }
            else if (r->seen.mode == seen_travel) {
                /* when we travel through a region, then we must add
                 * the factions of any units we saw */
                add_travelthru_addresses(r, ctx->f, &flist, stealthmod);
            }
            else if (r->seen.mode > seen_travel) {
                const unit *u = r->units;
                while (u != NULL) {
                    if (u->faction != ctx->f) {
                        faction *sf = visible_faction(ctx->f, u, get_otherfaction(u));
                        bool ballied = sf && sf != ctx->f && sf != lastf
                            && !fval(u, UFL_ANON_FACTION) && cansee(ctx->f, r, u, stealthmod);
                        if (ballied || is_allied(ctx->f, sf)) {
                            add_seen_faction_i(&flist, sf);
                            lastf = sf;
                        }
                    }
                    u = u->next;
                }
            }
        }
    }

    if (f_get_alliance(ctx->f)) {
        faction *f2;
        for (f2 = factions; f2; f2 = f2->next) {
            if (f2->alliance == ctx->f->alliance) {
                add_seen_faction_i(&flist, f2);
            }
        }
    }
    ctx->addresses = flist;
}

typedef struct report_type {
    struct report_type *next;
    report_fun write;
    const char *extension;
    int flag;
} report_type;

static report_type *report_types;

void register_reporttype(const char *extension, report_fun write, int flag)
{
    report_type *type = (report_type *)malloc(sizeof(report_type));
    if (!type) abort();
    type->extension = extension;
    type->write = write;
    type->flag = flag;
    type->next = report_types;
    report_types = type;
}

void reports_done(void) {
    report_type **rtp = &report_types;
    while (*rtp) {
        report_type *rt = *rtp;
        *rtp = rt->next;
        free(rt);
    }
}

size_t get_regions_distance_arr(region *rc, int radius, region *result[], size_t size)
{
    size_t n = 1, i;

    if (size > 0) {
        result[0] = rc;
        fset(rc, RF_MARK);
    }
    for (i = 0; i != n; ++i) {
        region *r;
        int dist;

        r = result[i];
        dist = distance(rc, r);
        if (dist < radius) {
            region *adj[MAXDIRECTIONS];
            int d;

            get_neighbours(r, adj);
            for (d = 0; d != MAXDIRECTIONS; ++d) {
                r = adj[d];
                if (r) {
                    if (size > n) {
                        if (!fval(r, RF_MARK) && dist < distance(rc, r)) {
                            result[n++] = r;
                            fset(r, RF_MARK);
                        }
                    }
                    else {
                        return -1;
                    }
                }
            }
        }
    }
    for (i = 0; i != n; ++i) {
        freset(result[i], RF_MARK);
    }
    return n;
}

region **get_regions_distance(region * root, int radius)
{
    region** arr = NULL;
    ptrdiff_t qi;

    arrput(arr, root);
    fset(root, RF_MARK);

    for (qi = 0; qi != arrlen(arr); ++qi) {
        region *r = arr[qi];
        region *next[MAXDIRECTIONS];
        int d;
        get_neighbours(r, next);

        for (d = 0; d != MAXDIRECTIONS; ++d) {
            if (next[d] && !fval(next[d], RF_MARK) && distance(next[d], root) <= radius) {
                r = next[d];
                arrput(arr, r);
                fset(r, RF_MARK);
            }
        }
    }
    for (qi = arrlen(arr); qi > 0; --qi) {
        region *r = arr[qi - 1];
        freset(r, RF_MARK);
    }
    return arr;
}

static void add_seen(region *r, seen_mode mode) {
    if (r->seen.mode < mode) {
        r->seen.mode = mode;
    }
}

static void add_seen_nb(faction *f, region *r, seen_mode mode) {
    region *first = r, *last = r;
    add_seen(r, mode);
    if (mode > seen_neighbour) {
        region *next[MAXDIRECTIONS];
        int d;
        get_neighbours(r, next);
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *rn = next[d];
            if (rn && rn->seen.mode < seen_neighbour) {
                rn->seen.mode = seen_neighbour;
                if (first->index > rn->index) first = rn;
                if (last->index < rn->index) last = rn;
            }
        }
    }
    update_interval(f, first);
    update_interval(f, last);
}

static void add_seen_lighthouse(region *r, faction *f)
{
    if (r->terrain->flags & SEA_REGION) {
        add_seen_nb(f, r, seen_lighthouse);
    }
    else {
        add_seen_nb(f, r, seen_lighthouse_land);
    }
}

static void add_seen_from_lighthouses(faction* f, region** arr, size_t len)
{
    size_t i;
    for (i = 0; i != len; ++i) {
        add_seen_lighthouse(arr[i], f);
    }
}

static void prepare_lighthouse(faction *f, region *r, int range)
{
    if (range > 3) {
        region ** arr = get_regions_distance(r, range);
        add_seen_from_lighthouses(f, arr, arrlenu(arr));
        arrfree(arr);
    }
    else {
        region *result[64];
        size_t n;

        n = get_regions_distance_arr(r, range, result, 64);
        assert(n > 0 && n <= 64);
        add_seen_from_lighthouses(f, result, n);
    }
}

static region *lastregion(faction * f)
{
    if (!f->units) {
        return NULL;
    }
    return f->last ? f->last->next : NULL;
}

static region *firstregion(faction * f)
{
    region *r = f->first;

    if (f->units == NULL)
        return NULL;
    if (r != NULL)
        return r;

    return f->first = regions;
}

static void cb_add_seen(region *r, const unit *u, void *cbdata) {
    faction *f = (faction *)cbdata;
    if (u->faction == f) {
        add_seen_nb(f, r, seen_travel);
    }
}

void report_warnings(faction *f, int now)
{
    int age = faction_age(f);
    if (age < NewbieImmunity()) {
        if (age + 2 < NewbieImmunity()) {
            ADDMSG(&f->msgs, msg_message("newbieimmunity", "turns",
                NewbieImmunity() - age - 1));
        } else if (age + 1 < NewbieImmunity()) {
            ADDMSG(&f->msgs, msg_message("newbieimmunityending", ""));
        } else {
            ADDMSG(&f->msgs, msg_message("newbieimmunityended", ""));
        }
    }

    if (f->race == get_race(RC_INSECT)) {
        season_t season = calendar_season(now + 1);

        if (season == SEASON_WINTER) {
            ADDMSG(&f->msgs, msg_message("nr_insectwinter", ""));
        }
        else if (season == SEASON_AUTUMN) {
            /* warning: next turn is the last week of autumn */
            if (calendar_season(now + 2) == SEASON_WINTER) {
                ADDMSG(&f->msgs, msg_message("nr_insectfall", ""));
            }
        }
    }
}

/** set region.seen based on visibility by one faction.
 *
 * this function may also update ctx->last and ctx->first for potential
 * lighthouses and travelthru reports
 */
void prepare_report(report_context *ctx, faction *f, const char *password)
{
    region *r;
    static int config;
    static bool rule_region_owners;
    static bool rule_lighthouse_units;
    const struct building_type *bt_lighthouse = bt_find("lighthouse");

    /* Insekten-Winter-Warnung */
    report_warnings(f, turn);

    if (bt_lighthouse && config_changed(&config)) {
        rule_region_owners = config_token("rules.region_owner_pay_building", bt_lighthouse->_name);
        rule_lighthouse_units = config_get_int("rules.lighthouse.unit_capacity", 0) != 0;
    }

    ctx->password = password;
    ctx->f = f;
    ctx->report_time = time(NULL);
    ctx->addresses = NULL;
    ctx->userdata = NULL;
    /* [first,last) interval of regions with a unit in it: */
    if (f->units) {
        ctx->first = firstregion(f);
        ctx->last = lastregion(f);

        for (r = ctx->first; r != ctx->last; r = r->next) {
            unit *u;
            building *b;
            int c = 0, range = 0;
            if (fval(r, RF_OBSERVER)) {
                int skill = get_observer(r, f);
                if (skill >= 0) {
                    add_seen_nb(f, r, seen_spell);
                }
            }
            if (fval(r, RF_LIGHTHOUSE)) {
                /* region owners get the report from lighthouses */
                if (rule_region_owners && f == region_get_owner(r)) {
                    for (b = rbuildings(r); b; b = b->next) {
                        if (b && b->type == bt_lighthouse) {
                            /* region owners get maximum range */
                            int lhr = lighthouse_view_distance(b, NULL);
                            if (lhr > range) range = lhr;
                        }
                    }
                }
            }

            b = NULL;
            for (u = r->units; u; u = u->next) {
                /* if we have any unit in this region, then we get seen_unit access */
                if (u->faction == f) {
                    add_seen_nb(f, r, seen_unit);
                    /* units inside the lighthouse get range based on their perception
                     * or the size, if perception is not a skill
                     */
                    if (!fval(r, RF_LIGHTHOUSE)) {
                        /* it's enough to add the region once, and if there are
                         * no lighthouses here, there is no need to look at more units */
                        break;
                    }
                }
                if (u->building && u->building->type == bt_lighthouse) {
                    if (u->building && b != u->building) {
                        b = u->building;
                        c = buildingcapacity(b);
                    }
                    if (rule_lighthouse_units) {
                        --c;
                    }
                    else {
                        c -= u->number;
                    }
                    if (u->faction == f && c >= 0) {
                        /* unit is one of ours, and inside the current lighthouse */
                        int br = lighthouse_view_distance(b, u);
                        if (br > range) {
                            range = br;
                        }
                    }
                }
            }
            if (range > 0) {
                /* we are in at least one lighthouse. add the regions we can see from here! */
                prepare_lighthouse(f, r, range);
            }

            if (fval(r, RF_TRAVELUNIT) && r->seen.mode < seen_travel) {
                travelthru_map(r, cb_add_seen, f);
            }
        }
    }
    /* [fast,last) interval of seen regions (with lighthouses and travel)
     * TODO: what about neighbours? when are they included? do we need
     * them outside of the CR? */
    ctx->first = firstregion(f);
    ctx->last = lastregion(f);
}

void finish_reports(report_context *ctx) {
    region *r;
    selist_free(ctx->addresses);
    for (r = ctx->first; r != ctx->last; r = r->next) {
        r->seen.mode = seen_none;
    }
}

int write_reports(faction * f, int options, const char *password)
{
    bool gotit = false;
    struct report_context ctx;
    const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf, 0 };
    report_type *rtype;

    if (noreports) {
        return false;
    }
    prepare_report(&ctx, f, password);
    get_addresses(&ctx);
    log_debug("Reports for %s", factionname(f));
    for (rtype = report_types; rtype != NULL; rtype = rtype->next) {
        if (options & rtype->flag) {
            int error = 0;
            do {
                char filename[32];
                char path[4096];
                sprintf(filename, "%d-%s.%s", turn, itoa36(f->no),
                    rtype->extension);
                path_join(reportpath(), filename, path, sizeof(path));
                errno = 0;
                if (rtype->write(path, &ctx, (const char *)utf8_bom) == 0) {
                    gotit = true;
                }
                if (errno) {
                    error = errno;
                    log_fatal("error %d writing report %s for faction %s: %s", errno, path, factionname(f), strerror(error));
                    errno = 0;
                }
            } while (error);
        }
    }
    if (!gotit) {
        log_warning("No report for faction %s!", itoa36(f->no));
    }
    finish_reports(&ctx);
    return 0;
}

static void write_script(FILE * F, const faction * f)
{
    report_type *rtype;
    char buf[1024];
    
    if (check_email(faction_getemail(f)) != 0) {
        return;
    }
    
    fprintf(F, "faction=%s:email=%s:lang=%s", itoa36(f->no), faction_getemail(f),
        locale_name(f->locale));
    if (f->options & (1 << O_BZIP2))
        fputs(":compression=bz2", F);
    else
        fputs(":compression=zip", F);

    fputs(":reports=", F);
    buf[0] = 0;
    for (rtype = report_types; rtype != NULL; rtype = rtype->next) {
        if (f->options & rtype->flag) {
            if (buf[0]) {
                str_strlcat(buf, ",", sizeof(buf));
            }
            str_strlcat(buf, rtype->extension, sizeof(buf));
        }
    }
    fputs(buf, F);
    fputc('\n', F);
}

int init_reports(void)
{
    region *r;
    bool update = true;
    create_directories();
    for (r = regions; r; r = r->next) {
        if (update) {
            update = update_lighthouses(r);
        }
        reorder_units(r);
    }
    return 0;
}

int reports(const char *filename)
{
    faction *f;
    FILE *mailit = NULL;
    int retval = 0;
    char buffer[PASSWORD_MAXSIZE];

    log_info("Writing reports for turn %d:", turn);
    report_donations();
    remove_empty_units();

    if (filename) {
        mailit = fopen(filename, "w");
        if (mailit == NULL) {
            log_error("%s could not be opened!\n", filename);
        }
    }

    for (f = factions; f; f = f->next) {
        if (f->email && !fval(f, FFL_NPC)) {
            char* password = NULL;
            if (f->lastorders == 0 || faction_age(f) <= 1) {
                /* neue Parteien, oder solche die noch NIE einen Zug gemacht haben,
                 * kriegen ein neues Passwort: */
                password = faction_genpassword(f, buffer);
            }
            int error = write_reports(f, f->options, password);
            if (error)
                retval = error;
            if (mailit)
                write_script(mailit, f);
        }
    }
    if (mailit)
        fclose(mailit);
    return retval;
}

static variant var_copy_string(variant x)
{
    x.v = x.v ? str_strdup((const char *)x.v) : 0;
    return x;
}

static void var_free_string(variant x)
{
    free(x.v);
}

static variant var_copy_order(variant x)
{
    x.v = copy_order((order *)x.v);
    return x;
}

static void var_free_order(variant x)
{
    free_order(x.v);
}

static variant var_copy_items(variant x)
{
    item *isrc;
    resource *rdst = NULL, **rptr = &rdst;

    for (isrc = (item *)x.v; isrc != NULL; isrc = isrc->next) {
        resource *res = malloc(sizeof(resource));
        if (!res) abort();
        res->number = isrc->number;
        res->type = isrc->type->rtype;
        *rptr = res;
        rptr = &res->next;
    }
    *rptr = NULL;
    x.v = rdst;
    return x;
}

static variant var_copy_resources(variant x)
{
    resource *rsrc;
    resource *rdst = NULL, **rptr = &rdst;

    for (rsrc = (resource *)x.v; rsrc != NULL; rsrc = rsrc->next) {
        resource *res = malloc(sizeof(resource));
        if (!res) abort();
        res->number = rsrc->number;
        res->type = rsrc->type;
        *rptr = res;
        rptr = &res->next;
    }
    *rptr = NULL;
    x.v = rdst;
    return x;
}

static void var_free_resources(variant x)
{
    resource *rsrc = (resource *)x.v;
    while (rsrc) {
        resource *res = rsrc->next;
        free(rsrc);
        rsrc = res;
    }
    x.v = 0;
}

static variant var_copy_regions(variant x)
{
    arg_regions *src = (arg_regions *)x.v;
    if (src) {
        arg_regions *dst;

        x.v = dst = malloc(sizeof(arg_regions));
        assert(dst);
        dst->nregions = src->nregions;
        src->nregions = 0;
        dst->regions = src->regions;
        src->regions = NULL;
    }
    return x;
}

static void var_free_regions(variant x) /*-V524 */
{
    arg_regions *arg = (arg_regions *)x.v;
    if (arg) {
        free(arg->regions);
    }
}

const char *trailinto(const region * r, const struct locale *lang)
{
    if (r) {
        static char ref[32];
        const char *s;
        const char *tname = terrain_name(r);
        size_t sz;

        sz = str_strlcpy(ref, tname, sizeof(ref));
        sz += str_strlcat(ref + sz, "_trail", sizeof(ref) - sz);
        s = LOC(lang, ref);
        if (s && *s) {
            if (strstr(s, "%s"))
                return s;
        }
    }
    return "%s";
}

size_t
f_regionid(const region * r, const faction * f, char *buffer, size_t size)
{
    size_t len;
    if (!r) {
        len = str_strlcpy(buffer, "(Chaos)", size);
    }
    else {
        plane *pl = rplane(r);
        const char *name = pl ? pl->name : 0;
        int nx = r->x, ny = r->y;
        int named = (name && name[0]);
        pnormalize(&nx, &ny, pl);
        adjust_coordinates(f, &nx, &ny, pl);
        len = str_strlcpy(buffer, rname(r, f ? f->locale : 0), size);
        snprintf(buffer + len, size - len, " (%d,%d%s%s)", nx, ny, named ? "," : "", (named) ? name : "");
        buffer[size - 1] = 0;
        len = strlen(buffer);
    }
    return len;
}

static char *f_regionid_s(const region * r, const faction * f)
{
    static char buf[NAMESIZE + 20]; /* FIXME: static return value */

    f_regionid(r, f, buf, sizeof(buf));
    return buf;
}

/*** BEGIN MESSAGE RENDERING ***/
static void eval_localize(struct opstack **stack, const void *userdata)
{                               /* (string, locale) -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct locale *lang = f ? f->locale : default_locale;
    const char *c = (const char *)opop_v(stack);
    c = LOC(lang, c);
    opush_v(stack, strcpy(balloc(strlen(c) + 1), c));
}

static void eval_trailto(struct opstack **stack, const void *userdata)
{                               /* (int, int) -> int */
    const struct faction *f = (const struct faction *)userdata;
    const struct locale *lang = f ? f->locale : default_locale;
    const struct region *r = (const struct region *)opop(stack).v;
    const char *trail = trailinto(r, lang);
    const char *rn = f_regionid_s(r, f);
    variant var;
    char *x = var.v = balloc(strlen(trail) + strlen(rn));
    sprintf(x, trail, rn);
    opush(stack, var);
}

static void eval_unit(struct opstack **stack, const void *userdata)
{                               /* unit -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct unit *u = (const struct unit *)opop(stack).v;
    const char *c = u ? unitname(u) : LOC(f->locale, "an_unknown_unit");
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_unit_dative(struct opstack **stack, const void *userdata)
{                               /* unit -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct unit *u = (const struct unit *)opop(stack).v;
    const char *c = u ? unitname(u) : LOC(f->locale, "unknown_unit_dative");
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_spell(struct opstack **stack, const void *userdata)
{                               /* unit -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct spell *sp = (const struct spell *)opop(stack).v;
    const char *c =
        sp ? spell_name(mkname_spell(sp), f->locale) : LOC(f->locale, "an_unknown_spell");
    variant var;

    assert(c || !"spell without description!");
    var.v = strcpy(balloc(strlen(c) + 1), c);
    opush(stack, var);
}

static void eval_curse(struct opstack **stack, const void *userdata)
{                               /* unit -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct curse_type *sp = (const struct curse_type *)opop(stack).v;
    const char *c =
        sp ? curse_name(sp, f->locale) : LOC(f->locale, "an_unknown_curse");
    variant var;

    assert(c || !"spell effect without description!");
    var.v = strcpy(balloc(strlen(c) + 1), c);
    opush(stack, var);
}

static void eval_unitid(struct opstack **stack, const void *userdata)
{                               /* unit -> int */
    const struct faction *f = (const struct faction *)userdata;
    const struct unit *u = (const struct unit *)opop(stack).v;
    const char *c = u ? unit_getname(u) : LOC(f->locale, "an_unknown_unit");
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_unitsize(struct opstack **stack, const void *userdata)
{                               /* unit -> int */
    const struct unit *u = (const struct unit *)opop(stack).v;
    variant var;

    UNUSED_ARG(userdata);
    var.i = u->number;
    opush(stack, var);
}

static void eval_faction(struct opstack **stack, const void *userdata)
{                               /* faction -> string */
    const struct faction *f = (const struct faction *)opop(stack).v;
    const char *c = factionname(f);
    size_t len = strlen(c);
    variant var;

    UNUSED_ARG(userdata);
    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_alliance(struct opstack **stack, const void *userdata)
{                               /* faction -> string */
    const struct alliance *al = (const struct alliance *)opop(stack).v;
    const char *c = alliancename(al);
    variant var;

    UNUSED_ARG(userdata);
    if (c != NULL) {
        size_t len = strlen(c);
        var.v = strcpy(balloc(len + 1), c);
    }
    else
        var.v = NULL;
    opush(stack, var);
}

static void eval_region(struct opstack **stack, const void *userdata)
{                               /* region -> string */
    char name[NAMESIZE + 32];
    const struct faction *f = (const struct faction *)userdata;
    const struct region *r = (const struct region *)opop(stack).v;
    const char *c = write_regionname(r, f, name, sizeof(name));
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_terrain(struct opstack **stack, const void *userdata)
{                               /* region -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct region *r = (const struct region *)opop(stack).v;
    const char *c = LOC(f->locale, terrain_name(r));
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_ship(struct opstack **stack, const void *userdata)
{                               /* ship -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct ship *u = (const struct ship *)opop(stack).v;
    const char *c = u ? shipname(u) : LOC(f->locale, "an_unknown_ship");
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_building(struct opstack **stack, const void *userdata)
{                               /* building -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct building *u = (const struct building *)opop(stack).v;
    const char *c = u ? buildingname(u) : LOC(f->locale, "an_unknown_building");
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_weight(struct opstack **stack, const void *userdata)
{                               /* region -> string */
    char buffer[32];
    const struct faction *f = (const struct faction *)userdata;
    const struct locale *lang = f->locale;
    int weight = opop_i(stack);
    variant var;

    if (weight % SCALEWEIGHT == 0) {
        if (weight == SCALEWEIGHT) {
            sprintf(buffer, "1 %s", LOC(lang, "weight_unit"));
        }
        else {
            sprintf(buffer, "%d %s", weight / SCALEWEIGHT, LOC(lang,
                "weight_unit_p"));
        }
    }
    else {
        if (weight == 1) {
            sprintf(buffer, "1 %s %d", LOC(lang, "weight_per"), SCALEWEIGHT);
        }
        else {
            sprintf(buffer, "%d %s %d", weight, LOC(lang, "weight_per_p"),
                SCALEWEIGHT);
        }
    }

    var.v = strcpy(balloc(strlen(buffer) + 1), buffer);
    opush(stack, var);
}

static void eval_resource(struct opstack **stack, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const struct locale *lang = report ? report->locale : default_locale;
    int j = opop(stack).i;
    const struct resource_type *res = (const struct resource_type *)opop(stack).v;
    const char *name = resourcename(res, j != 1);
    const char *c = LOC(lang, name);
    variant var;
    if (c) {
        size_t len = strlen(c);

        var.v = strcpy(balloc(len + 1), c);
    } else {
        log_error("missing translation for %s in eval_resource", name);
        var.v = NULL;
    }
    opush(stack, var);
}

static void eval_race(struct opstack **stack, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const struct locale *lang = report ? report->locale : default_locale;
    int j = opop(stack).i;
    const race *r = (const race *)opop(stack).v;
    const char *name = rc_name_s(r, (j == 1) ? NAME_SINGULAR : NAME_PLURAL);
    const char *c = LOC(lang, name);
    variant var;
    if (c) {
        size_t len = strlen(c);
        var.v = strcpy(balloc(len + 1), c);
    }
    else {
        log_error("missing translation for %s in eval_race", name);
        var.v = NULL;
    }
    opush(stack, var);
}

static void eval_order(struct opstack **stack, const void *userdata)
{                               /* order -> string */
    const faction *f = (const faction *)userdata;
    const struct order *ord = (const struct order *)opop(stack).v;
    char buf[4096];
    size_t len;
    variant var;
    const struct locale *lang = f ? f->locale : default_locale;

    UNUSED_ARG(userdata);
    write_order(ord, lang, buf, sizeof(buf));
    len = strlen(buf);
    var.v = strcpy(balloc(len + 1), buf);
    opush(stack, var);
}

void report_battle_start(battle * b)
{
    bfaction *bf;

    for (bf = b->factions; bf; bf = bf->next) {
        message *m;
        faction *f = bf->faction;
        const char *lastf = NULL;
        bool first = false;
        size_t si, sl = arrlen(b->sides);
        gbString str = gb_make_string_length(NULL, 0);

        for (si = 0; si != sl; ++si) {
            side* s = b->sides[si];
            fighter *df;
            for (df = s->fighters; df; df = df->next) {
                if (is_attacker(df)) {
                    if (first) {
                        str = gb_append_string_length(str, ", ", 2);
                    }
                    if (lastf) {
                        str = gb_append_cstring(str, lastf);
                        first = true;
                    }
                    if (seematrix(f, s))
                        lastf = sidename(s);
                    else
                        lastf = LOC(f->locale, "unknown_faction_dative");
                    break;
                }
            }
        }
        if (lastf) {
            if (first) {
                str = gb_append_string_length(str, " ", 1);
                str = gb_append_cstring(str, LOC(f->locale, "and"));
                str = gb_append_string_length(str, " ", 1);
            }
            str = gb_append_cstring(str, lastf);
        }

        m = msg_message("start_battle", "factions", str);
        gb_free_string(str);
        battle_message_faction(b, f, m);
        msg_release(m);
    }
}

static void eval_resources(struct opstack **stack, const void *userdata)
{                               /* order -> string */
    const faction *f = (const faction *)userdata;
    const struct locale *lang = f ? f->locale : default_locale;
    const struct resource *res = (const struct resource *)opop(stack).v;
    char buf[1024];        /* but we only use about half of this */
    variant var;
    sbstring sbs;
    
    sbs_init(&sbs, buf, sizeof(buf));
    while (res != NULL) {
        const char *rname =
            resourcename(res->type, (res->number != 1) ? NMF_PLURAL : 0);
        sbs_strcat(&sbs, str_itoa(res->number));
        sbs_strcat(&sbs, " ");
        sbs_strcat(&sbs, LOC(lang, rname));

        res = res->next;
        if (res != NULL) {
            sbs_strcat(&sbs, ", ");
        }
    }
    var.v = strcpy(balloc(sbs_length(&sbs)), buf);
    opush(stack, var);
}

static void eval_regions(struct opstack **stack, const void *userdata)
{                               /* order -> string */
    const faction *report = (const faction *)userdata;
    int i = opop(stack).i;
    int handle_end, begin = opop(stack).i;
    const arg_regions *aregs = (const arg_regions *)opop(stack).v;
    char buf[256];
    variant var;
    sbstring sbs;

    sbs_init(&sbs, buf, sizeof(buf));
    if (aregs == NULL) {
        handle_end = begin;
    }
    else if (i >= 0) {
        handle_end = begin + i;
    }
    else {
        handle_end = aregs->nregions + i;
    }

    for (i = begin; i < handle_end; ++i) {
        const char *rname = (const char *)regionname(aregs->regions[i], report);
        sbs_strcat(&sbs, rname);

        if (i + 1 < handle_end) {
            sbs_strcat(&sbs, ", ");
        }
    }
    var.v = strcpy(balloc(sbs_length(&sbs)), buf);
    opush(stack, var);
}

const char *get_mailcmd(const struct locale *loc)
{
    static char result[64]; /* FIXME: static return buffer */
    snprintf(result, sizeof(result), "%s %d %s", game_mailcmd(), game_id(), LOC(loc, "mailcmd"));
    return result;
}

static void print_trail(const faction *f, const region *r,
    const struct locale *lang, struct sbstring *sbp)
{
    char buf[64];
    const char *trail = trailinto(r, lang);
    const char *rn = f_regionid_s(r, f);
    if (snprintf(buf, sizeof(buf), trail, rn) != 0) {
        sbs_strcat(sbp, buf);
    }
}

static void eval_trail(struct opstack **stack, const void *userdata)
{                               /* order -> string */
    const faction *report = (const faction *)userdata;
    const struct locale *lang = report ? report->locale : default_locale;
    const arg_regions *aregs = (const arg_regions *)opop(stack).v;
    char buf[512];
    variant var;
    sbstring sbs;
#ifdef _SECURECRT_ERRCODE_VALUES_DEFINED
    /* MSVC touches errno in snprintf */
    int eold = errno;
#endif

    sbs_init(&sbs, buf, sizeof(buf));
    if (aregs != NULL) {
        int i, handle_end = 0, begin = 0;
        handle_end = aregs->nregions;
        for (i = begin; i < handle_end; ++i) {
            region *r = aregs->regions[i];

            print_trail(report, r, lang, &sbs);

            if (i + 2 < handle_end) {
                sbs_strcat(&sbs, ", ");
            }
            else if (i + 1 < handle_end) {
                sbs_strcat(&sbs, LOC(lang, "list_and"));
            }
        }
    }
    var.v = strcpy(balloc(sbs_length(&sbs)), buf);
    opush(stack, var);
#ifdef _SECURECRT_ERRCODE_VALUES_DEFINED
    if (errno == ERANGE) {
        errno = eold;
    }
#endif
}

void report_race_skills(const race *rc, const struct locale *lang, sbstring *sbp)
{
    int dh = 0, dh1 = 0, sk;

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
                    sbs_strcat(sbp, LOC(lang, "list_and"));
                }
                else {
                    sbs_strcat(sbp, ", ");
                }
            }
            sbs_strcat(sbp, skillname((skill_t)sk, lang));
        }
    }
}

static void eval_direction(struct opstack **stack, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const struct locale *lang = report ? report->locale : default_locale;
    int i = opop(stack).i;
    const char *c = LOC(lang, (i >= 0) ? directions[i] : "unknown_direction");
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_skill(struct opstack **stack, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const struct locale *lang = report ? report->locale : default_locale;
    skill_t sk = (skill_t)opop(stack).i;
    const char *c = skillname(sk, lang);
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_int36(struct opstack **stack, const void *userdata)
{
    int i = opop(stack).i;
    const char *c = itoa36(i);
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
    UNUSED_ARG(userdata);
}

/*** END MESSAGE RENDERING ***/

int stream_printf(struct stream * out, const char *format, ...)
{
    va_list args;
    int result;
    char buffer[4096];
    size_t bytes = sizeof(buffer);
    /* TODO: should be in storage/stream.c (doesn't exist yet) */
    va_start(args, format);
    result = vsnprintf(buffer, bytes, format, args);
    if (result >= 0 && (size_t)result < bytes) {
        bytes = (size_t)result;
        /* TODO: else = buffer too small */
    }
    out->api->write(out->handle, buffer, bytes);
    va_end(args);
    return result;
}

typedef struct count_data {
    int n;
    const struct faction *f;
} count_data;

static void count_cb(region *r, const unit *u, void *cbdata) {
    count_data *data = (count_data *)cbdata;
    const struct faction *f = data->f;
    if (r != u->region && (!u->ship || ship_owner(u->ship) == u)) {
        if (cansee(f, r, u, 0)) {
            ++data->n;
        }
    }
}

int count_travelthru(struct region *r, const struct faction *f) {
    count_data data = { 0 };
    data.f = f;
    travelthru_map(r, count_cb, &data);
    return data.n;
}

bool visible_unit(const unit *u, const faction *f, int stealthmod, enum seen_mode mode)
{
    if (u->faction == f) {
        return true;
    }
    else if (mode >= seen_lighthouse && stealthmod > INT_MIN) {
        return cansee(f, u->region, u, stealthmod);
    }
    return false;
}

void register_reports(void)
{
    /* register datatypes for the different message objects */
    register_argtype("alliance", NULL, NULL, VAR_VOIDPTR);
    register_argtype("building", NULL, NULL, VAR_VOIDPTR);
    register_argtype("direction", NULL, NULL, VAR_INT);
    register_argtype("faction", NULL, NULL, VAR_VOIDPTR);
    register_argtype("race", NULL, NULL, VAR_VOIDPTR);
    register_argtype("region", NULL, NULL, VAR_VOIDPTR);
    register_argtype("resource", NULL, NULL, VAR_VOIDPTR);
    register_argtype("ship", NULL, NULL, VAR_VOIDPTR);
    register_argtype("skill", NULL, NULL, VAR_VOIDPTR);
    register_argtype("spell", NULL, NULL, VAR_VOIDPTR);
    register_argtype("curse", NULL, NULL, VAR_VOIDPTR);
    register_argtype("unit", NULL, NULL, VAR_VOIDPTR);
    register_argtype("int", NULL, NULL, VAR_INT);
    register_argtype("string", var_free_string, var_copy_string, VAR_VOIDPTR);
    register_argtype("order", var_free_order, var_copy_order, VAR_VOIDPTR);
    register_argtype("resources", var_free_resources, var_copy_resources, VAR_VOIDPTR);
    register_argtype("items", var_free_resources, var_copy_items, VAR_VOIDPTR);
    register_argtype("regions", var_free_regions, var_copy_regions, VAR_VOIDPTR);

    /* register functions that turn message contents to readable strings */
    add_function("alliance", &eval_alliance);
    add_function("region", &eval_region);
    add_function("terrain", &eval_terrain);
    add_function("weight", &eval_weight);
    add_function("resource", &eval_resource);
    add_function("race", &eval_race);
    add_function("faction", &eval_faction);
    add_function("ship", &eval_ship);
    add_function("unit", &eval_unit);
    add_function("unit.dative", &eval_unit_dative);
    add_function("unit.id", &eval_unitid);
    add_function("unit.size", &eval_unitsize);
    add_function("building", &eval_building);
    add_function("skill", &eval_skill);
    add_function("order", &eval_order);
    add_function("direction", &eval_direction);
    add_function("int36", &eval_int36);
    add_function("trailto", &eval_trailto);
    add_function("localize", &eval_localize);
    add_function("spell", &eval_spell);
    add_function("curse", &eval_curse);
    add_function("resources", &eval_resources);
    add_function("regions", &eval_regions);
    add_function("trail", &eval_trail);
}
