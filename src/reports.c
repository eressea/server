/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>
#include "reports.h"
#include "laws.h"
#include "seen.h"
#include "travelthru.h"
#include "lighthouse.h"
#include "donations.h"

/* kernel includes */
#include <kernel/ally.h>
#include <kernel/alliance.h>
#include <kernel/connection.h>
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/base36.h>
#include <util/functions.h>
#include <util/translation.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <stream.h>
#include <quicklist.h>

/* libc includes */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

/* attributes includes */
#include <attributes/follow.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/stealth.h>

#include "move.h"

#if defined(_MSC_VER) && _MSC_VER >= 1900
# pragma warning(disable: 4774) // TODO: remove this
#endif

#define SCALEWEIGHT      100    /* Faktor, um den die Anzeige von Gewichten skaliert wird */

bool nocr = false;
bool nonr = false;
bool noreports = false;

const char *visibility[] = {
    "none",
    "neighbour",
    "lighthouse",
    "travel",
    "far",
    "unit",
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

static char *groupid(const struct group *g, const struct faction *f)
{
    typedef char name[OBJECTIDSIZE + 1];
    static name idbuf[8];
    static int nextbuf = 0;
    char *buf = idbuf[(++nextbuf) % 8];
    sprintf(buf, "%s (%s)", g->name, factionid(f));
    return buf;
}

const char *combatstatus[] = {
    "status_aggressive", "status_front",
    "status_rear", "status_defensive",
    "status_avoid", "status_flee"
};

size_t report_status(const unit * u, const struct locale *lang, char *fsbuf, size_t buflen)
{
    const char * status = LOC(lang, combatstatus[u->status]);
    size_t len = 0;

    if (!status) {
        const char *lname = locale_name(lang);
        struct locale *wloc = get_or_create_locale(lname);
        log_error("no translation for combat status %s in %s", combatstatus[u->status], lname);
        locale_setstring(wloc, combatstatus[u->status], combatstatus[u->status]);
        len = strlcpy(fsbuf, combatstatus[u->status], buflen);
    }
    else {
        len = strlcpy(fsbuf, status, buflen);
    }
    if (fval(u, UFL_NOAID)) {
        len += strlcat(fsbuf+len, ", ", buflen-len);
        len += strlcat(fsbuf+len, LOC(lang, "status_noaid"), buflen-len);
    }

    return len;
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
static size_t buforder(char *buffer, size_t size, const order * ord, int mode)
{
    char *bufp = buffer;

    bufp = STRLCPY(bufp, ", \"", size);
    if (mode < ORDERS_IN_NR) {
        char cmd[ORDERSIZE];
        get_command(ord, cmd, sizeof(cmd));
        bufp = STRLCPY(bufp, cmd, size);
    }
    else {
        bufp = STRLCPY(bufp, "...", size);
    }

    if (size > 1) {
        *bufp++ = '\"';
        --size;
    }
    else {
        WARN_STATIC_BUFFER();
    }

    return bufp-buffer;
}

/** create a report of a list of items to a non-owner.
 * \param result: an array of size items.
 * \param size: maximum number of items to return
 * \param owner: the owner of the items, or NULL for faction::items etc.
 * \param viewer: the faction looking at the items
 */
int
report_items(const item * items, item * result, int size, const unit * owner,
const faction * viewer)
{
    const item *itm;
    int n = 0;                    /* number of results */

    assert(owner == NULL || viewer != owner->faction
        || !"not required for owner=viewer!");
    assert(size);

    for (itm = items; itm; itm = itm->next) {
        item *ishow;
        const char *ic;

        report_item(owner, itm, viewer, NULL, &ic, NULL, false);
        if (ic && *ic) {
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

static void
report_resource(resource_report * result, const char *name, int number,
int level)
{
    result->name = name;
    result->number = number;
    result->level = level;
}

void report_race(const struct unit *u, const char **name, const char **illusion)
{
    if (illusion) {
        const race *irace = u_irace(u);
        if (irace && irace != u_race(u)) {
            *illusion = irace->_name;
        }
        else {
            *illusion = NULL;
        }
    }
    if (name) {
        *name = u_race(u)->_name;
        if (fval(u_race(u), RCF_SHAPESHIFTANY)) {
            const char *str = get_racename(u->attribs);
            if (str)
                *name = str;
        }
    }
}

void
report_building(const struct building *b, const char **name,
const char **illusion)
{
    const struct building_type *bt_illusion;

    if (name) {
        *name = buildingtype(b->type, b, b->size);
    }
    if (illusion) {
        *illusion = NULL;

        bt_illusion = bt_find("illusioncastle");
        if (bt_illusion && b->type == bt_illusion) {
            const attrib *a = a_findc(b->attribs, &at_icastle);
            if (a != NULL) {
                *illusion = buildingtype(icastle_type(a), b, b->size);
            }
        }
    }
}

int
report_resources(const seen_region * sr, resource_report * result, int size,
const faction * viewer)
{
    const region *r = sr->r;
    int n = 0;

    if (r->land) {
        int peasants = rpeasants(r);
        int money = rmoney(r);
        int horses = rhorses(r);
        int trees = rtrees(r, 2);
        int saplings = rtrees(r, 1);
        bool mallorn = fval(r, RF_MALLORN) != 0;

        if (money) {
            if (n >= size)
                return -1;
            report_resource(result + n, "rm_money", money, -1);
            ++n;
        }
        if (peasants) {
            if (n >= size)
                return -1;
            report_resource(result + n, "rm_peasant", peasants, -1);
            ++n;
        }
        if (horses) {
            if (n >= size)
                return -1;
            report_resource(result + n, "rm_horse", horses, -1);
            ++n;
        }
        if (saplings) {
            if (n >= size)
                return -1;
            report_resource(result + n, mallorn ? "rm_mallornsapling" : "rm_sapling",
                saplings, -1);
            ++n;
        }
        if (trees) {
            if (n >= size)
                return -1;
            report_resource(result + n, mallorn ? "rm_mallorn" : "rm_tree", trees,
                -1);
            ++n;
        }
    }

    if (sr->mode >= see_unit) {
        rawmaterial *res = r->resources;
        while (res) {
            int maxskill = 0;
            const item_type *itype = resource2item(res->type->rtype);
            int level = res->level + itype->construction->minskill - 1;
            int visible = -1;
            if (res->type->visible == NULL) {
                visible = res->amount;
                level = res->level + itype->construction->minskill - 1;
            }
            else {
                const unit *u;
                for (u = r->units; visible != res->amount && u != NULL; u = u->next) {
                    if (u->faction == viewer) {
                        int s = effskill(u, itype->construction->skill, 0);
                        if (s > maxskill) {
                            maxskill = s;
                            visible = res->type->visible(res, maxskill);
                        }
                    }
                }
            }
            if (level >= 0 && visible >= 0) {
                if (n >= size)
                    return -1;
                report_resource(result + n, res->type->name, visible, level);
                n++;
            }
            res = res->next;
        }
    }
    return n;
}

int
bufunit(const faction * f, const unit * u, unsigned int indent, int mode, char *buf,
size_t size)
{
    int i, dh;
    int getarnt = fval(u, UFL_ANON_FACTION);
    const char *pzTmp, *str;
    building *b;
    bool isbattle = (bool)(mode == see_battle);
    int telepath_see = 0;
    item *itm;
    item *show;
    faction *fv = visible_faction(f, u);
    char *bufp = buf;
    bool itemcloak = false;
    const curse_type *itemcloak_ct = 0;
    int result = 0;
    item results[MAX_INVENTORY];

    itemcloak_ct = ct_find("itemcloak");
    if (itemcloak_ct) {
        itemcloak = curse_active(get_curse(u->attribs, itemcloak_ct));
    }

    bufp = STRLCPY(bufp, unitname(u), size);

    if (!isbattle) {
        attrib *a_otherfaction = a_find(u->attribs, &at_otherfaction);
        if (u->faction == f) {
            if (fval(u, UFL_GROUP)) {
                attrib *a = a_find(u->attribs, &at_group);
                if (a) {
                    group *g = (group *)a->data.v;
                    bufp = STRLCPY(bufp, ", ", size);
                    bufp = STRLCPY(bufp, groupid(g, f), size);
                }
            }
            if (getarnt) {
                bufp = STRLCPY(bufp, ", ", size);
                bufp = STRLCPY(bufp, LOC(f->locale, "anonymous"), size);
            }
            else if (a_otherfaction) {
                faction *otherfaction = get_otherfaction(a_otherfaction);
                if (otherfaction) {
                    bufp = STRLCPY(bufp, ", ", size);
                    bufp = STRLCPY(bufp, factionname(otherfaction), size);
                }
            }
        }
        else {
            if (getarnt) {
                bufp = STRLCPY(bufp, ", ", size);
                bufp = STRLCPY(bufp, LOC(f->locale, "anonymous"), size);
            }
            else {
                if (a_otherfaction && alliedunit(u, f, HELP_FSTEALTH)) {
                    faction *f = get_otherfaction(a_otherfaction);
                    int result =
                        _snprintf(bufp, size, ", %s (%s)", factionname(f),
                        factionname(u->faction));
                    if (wrptr(&bufp, &size, result) != 0)
                        WARN_STATIC_BUFFER();
                }
                else {
                    bufp = STRLCPY(bufp, ", ", size);
                    bufp = STRLCPY(bufp, factionname(fv), size);
                }
            }
        }
    }

    bufp = STRLCPY(bufp, ", ", size);

    if (wrptr(&bufp, &size, _snprintf(bufp, size, "%d ", u->number)))
        WARN_STATIC_BUFFER();

    pzTmp = get_racename(u->attribs);
    if (pzTmp) {
        bufp = STRLCPY(bufp, pzTmp, size);
        if (u->faction == f && fval(u_race(u), RCF_SHAPESHIFTANY)) {
            bufp = STRLCPY(bufp, " (", size);
            bufp = STRLCPY(bufp, racename(f->locale, u, u_race(u)), size);
            if (size > 1) {
                strcpy(bufp++, ")");
                --size;
            }
        }
    }
    else {
        const race *irace = u_irace(u);
        bufp = STRLCPY(bufp, racename(f->locale, u, irace), size);
        if (u->faction == f && irace != u_race(u)) {
            bufp = STRLCPY(bufp, " (", size);
            bufp = STRLCPY(bufp, racename(f->locale, u, u_race(u)), size);
            if (size > 1) {
                strcpy(bufp++, ")");
                --size;
            }
        }
    }

    if (fval(u, UFL_HERO) && (u->faction == f || omniscient(f))) {
        bufp = STRLCPY(bufp, ", ", size);
        bufp = STRLCPY(bufp, LOC(f->locale, "hero"), size);
    }
    /* status */

    if (u->number && (u->faction == f || telepath_see || isbattle)) {
        const char *c = hp_status(u);
        c = c ? LOC(f->locale, c) : 0;
        bufp = STRLCPY(bufp, ", ", size);
        bufp += report_status(u, f->locale, bufp, size);
        if (c || fval(u, UFL_HUNGER)) {
            bufp = STRLCPY(bufp, " (", size);
            if (c) {
                bufp = STRLCPY(bufp, c, size);
            }
            if (fval(u, UFL_HUNGER)) {
                if (c) {
                    bufp = STRLCPY(bufp, ", ", size);
                }
                bufp = STRLCPY(bufp, LOC(f->locale, "unit_hungers"), size);
            }
            if (size > 1) {
                strcpy(bufp++, ")");
                --size;
            }
        }
    }
    if (is_guard(u, GUARD_ALL) != 0) {
        bufp = STRLCPY(bufp, ", ", size);
        bufp = STRLCPY(bufp, LOC(f->locale, "unit_guards"), size);
    }

    if ((b = usiege(u)) != NULL) {
        bufp = STRLCPY(bufp, ", belagert ", size);
        bufp = STRLCPY(bufp, buildingname(b), size);
    }

    dh = 0;
    if (u->faction == f || telepath_see) {
        skill *sv;
        for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
            size_t bytes = spskill(bufp, size, f->locale, u, sv, &dh, 1);
            assert(bytes <=INT_MAX);
            if (wrptr(&bufp, &size, (int)bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }

    dh = 0;
    if (f == u->faction || telepath_see || omniscient(f)) {
        show = u->items;
    }
    else if (!itemcloak && mode >= see_unit) {
        int n = report_items(u->items, results, MAX_INVENTORY, u, f);
        assert(n >= 0);
        if (n > 0)
            show = results;
        else
            show = NULL;
    }
    else {
        show = NULL;
    }
    for (itm = show; itm; itm = itm->next) {
        const char *ic;
        int in;
        report_item(u, itm, f, &ic, NULL, &in, false);
        if (in == 0 || ic == NULL)
            continue;
        bufp = STRLCPY(bufp, ", ", size);

        if (!dh) {
            result = _snprintf(bufp, size, "%s: ", LOC(f->locale, "nr_inventory"));
            if (wrptr(&bufp, &size, result) != 0)
                WARN_STATIC_BUFFER();
            dh = 1;
        }
        if (in == 1) {
            bufp = STRLCPY(bufp, ic, size);
        }
        else {
            if (wrptr(&bufp, &size, _snprintf(bufp, size, "%d %s", in, ic)))
                WARN_STATIC_BUFFER();
        }
    }

    if (u->faction == f || telepath_see) {
        spellbook *book = unit_get_spellbook(u);

        if (book) {
            quicklist *ql = book->spells;
            int qi, header, maxlevel = effskill(u, SK_MAGIC, 0);
            int result = _snprintf(bufp, size, ". Aura %d/%d", get_spellpoints(u), max_spellpoints(u->region, u));
            if (wrptr(&bufp, &size, result) != 0) {
                WARN_STATIC_BUFFER();
            }

            for (header = 0, qi = 0; ql; ql_advance(&ql, &qi, 1)) {
                spellbook_entry * sbe = (spellbook_entry *)ql_get(ql, qi);
                if (sbe->level <= maxlevel) {
                    int result = 0;
                    if (!header) {
                        result = _snprintf(bufp, size, ", %s: ", LOC(f->locale, "nr_spells"));
                        header = 1;
                    }
                    else {
                        result = (int)strlcpy(bufp, ", ", size);
                    }
                    if (wrptr(&bufp, &size, result) != 0) {
                        WARN_STATIC_BUFFER();
                    }
                    bufp = STRLCPY(bufp, spell_name(sbe->sp, f->locale), size);
                }
            }

            for (i = 0; i != MAXCOMBATSPELLS; ++i) {
                if (get_combatspell(u, i))
                    break;
            }
            if (i != MAXCOMBATSPELLS) {
                int result =
                    _snprintf(bufp, size, ", %s: ", LOC(f->locale, "nr_combatspells"));
                if (wrptr(&bufp, &size, result) != 0)
                    WARN_STATIC_BUFFER();

                dh = 0;
                for (i = 0; i < MAXCOMBATSPELLS; i++) {
                    const spell *sp;
                    if (!dh) {
                        dh = 1;
                    }
                    else {
                        bufp = STRLCPY(bufp, ", ", size);
                    }
                    sp = get_combatspell(u, i);
                    if (sp) {
                        int sl = get_combatspelllevel(u, i);
                        bufp = STRLCPY(bufp, spell_name(sp, u->faction->locale), size);
                        if (sl > 0) {
                            result = _snprintf(bufp, size, " (%d)", sl);
                            if (wrptr(&bufp, &size, result) != 0)
                                WARN_STATIC_BUFFER();
                        }
                    }
                    else {
                        bufp = STRLCPY(bufp, LOC(f->locale, "nr_nospells"), size);
                    }
                }
            }
        }
        if (!isbattle) {
            bool printed = 0;
            order *ord;;
            for (ord = u->old_orders; ord; ord = ord->next) {
                keyword_t kwd = getkeyword(ord);
                if (is_repeated(kwd)) {
                    if (printed < ORDERS_IN_NR) {
                        int result = (int)buforder(bufp, size, ord, printed++);
                        if (wrptr(&bufp, &size, result) != 0)
                            WARN_STATIC_BUFFER();
                    }
                    else
                        break;
                }
            }
            if (printed < ORDERS_IN_NR)
                for (ord = u->orders; ord; ord = ord->next) {
                    keyword_t kwd = getkeyword(ord);
                    if (is_repeated(kwd)) {
                        if (printed < ORDERS_IN_NR) {
                            int result = (int)buforder(bufp, size, ord, printed++);
                            if (wrptr(&bufp, &size, result) != 0)
                                WARN_STATIC_BUFFER();
                        }
                        else
                            break;
                    }
                }
        }
    }
    i = 0;

    str = u_description(u, f->locale);
    if (str) {
        bufp = STRLCPY(bufp, "; ", size);
        bufp = STRLCPY(bufp, str, size);
        i = str[strlen(str) - 1];
    }
    if (i != '!' && i != '?' && i != '.') {
        if (size > 1) {
            strcpy(bufp++, ".");
            --size;
        }
    }
    pzTmp = uprivate(u);
    if (u->faction == f && pzTmp) {
        bufp = STRLCPY(bufp, " (Bem: ", size);
        bufp = STRLCPY(bufp, pzTmp, size);
        bufp = STRLCPY(bufp, ")", size);
    }

    dh = 0;
    if (!getarnt && f) {
        if (alliedfaction(rplane(u->region), f, fv, HELP_ALL)) {
            dh = 1;
        }
    }
    if (size <= 1) {
        log_warning("bufunit ran out of space after writing %u bytes.\n", (bufp - buf));
    }
    return dh;
}

/* TODO: telepath_see wird nicht ber�cksichtigt: Parteien mit
 * telepath_see sollten immer einzelne Einheiten zu sehen
 * bekommen, alles andere ist darstellungsteschnisch kompliziert.
 */

size_t
spskill(char *buffer, size_t size, const struct locale * lang,
const struct unit * u, struct skill * sv, int *dh, int days)
{
    char *bufp = buffer;
    int i, effsk;

    if (!u->number)
        return 0;
    if (sv->level <= 0) {
        if (sv->old <= 0 || (u->faction->options & want(O_SHOWSKCHANGE)) == 0) {
            return 0;
        }
    }

    bufp = STRLCPY(bufp, ", ", size);

    if (!*dh) {
        bufp = STRLCPY(bufp, LOC(lang, "nr_skills"), size);
        bufp = STRLCPY(bufp, ": ", size);
        *dh = 1;
    }
    bufp = STRLCPY(bufp, skillname(sv->id, lang), size);
    bufp = STRLCPY(bufp, " ", size);

    if (sv->id == SK_MAGIC) {
        sc_mage *mage = get_mage(u);
        if (mage && mage->magietyp != M_GRAY) {
            bufp = STRLCPY(bufp, LOC(lang, mkname("school",
                magic_school[mage->magietyp])), size);
            bufp = STRLCPY(bufp, " ", size);
        }
    }

    if (sv->id == SK_STEALTH && fval(u, UFL_STEALTH)) {
        i = u_geteffstealth(u);
        if (i >= 0) {
            if (wrptr(&bufp, &size, _snprintf(bufp, size, "%d/", i)) != 0)
                WARN_STATIC_BUFFER();
        }
    }

    effsk = eff_skill(u, sv, 0);
    if (wrptr(&bufp, &size, _snprintf(bufp, size, "%d", effsk)) != 0)
        WARN_STATIC_BUFFER();

    if (u->faction->options & want(O_SHOWSKCHANGE)) {
        int oldeff = 0;
        int diff;

        if (sv->old > 0) {
            oldeff = sv->old + get_modifier(u, sv->id, sv->old, u->region, false);
        }

        oldeff = _max(0, oldeff);
        diff = effsk - oldeff;

        if (diff != 0) {
            if (wrptr(&bufp, &size, _snprintf(bufp, size, " (%s%d)", (diff > 0) ? "+" : "", diff)) != 0)
                WARN_STATIC_BUFFER();
        }
    }
    return bufp-buffer;
}

void split_paragraph(strlist ** SP, const char *s, unsigned int indent, unsigned int width, char mark)
{

    /* Die Liste SP wird mit dem String s aufgefuellt, mit indent und einer
    * mark, falls angegeben. SP wurde also auf 0 gesetzt vor dem Aufruf.
    * Vgl. spunit (). */
    bool firstline;
    static char buf[REPORTWIDTH + 1]; // FIXME: static buffer, artificial limit
    size_t len = strlen(s);

    assert(width <= REPORTWIDTH);
    width -= indent;
    firstline = (mark != 0 && indent > 2);
    *SP = 0;

    while (len > 0) {
        unsigned int j;
        const char *cut = 0, *space = strchr(s, ' ');
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
            cut = s + _min(len, REPORTWIDTH);
        }
        strncpy(buf + indent, s, cut - s);
        buf[indent + (cut - s)] = 0;
        addstrlist(SP, buf); // TODO: too much string copying, cut out this function
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

void lparagraph(struct strlist **SP, char *s, unsigned int indent, char mark)
{
    /* Die Liste SP wird mit dem String s aufgefuellt, mit indent und einer
     * mark, falls angegeben. SP wurde also auf 0 gesetzt vor dem Aufruf.
     * Vgl. spunit (). */

    char *buflocal = calloc(strlen(s) + indent + 1, sizeof(char));

    if (indent) {
        memset(buflocal, ' ', indent);
        if (mark)
            buflocal[indent - 2] = mark;
    }
    strcpy(buflocal + indent, s);
    addstrlist(SP, buflocal);
    free(buflocal);
}

void
spunit(struct strlist **SP, const struct faction *f, const unit * u, unsigned int indent,
int mode)
{
    char buf[DISPLAYSIZE];
    int dh = bufunit(f, u, indent, mode, buf, sizeof(buf));
    lparagraph(SP, buf, indent,
        (char)((u->faction == f) ? '*' : (dh ? '+' : '-')));
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
            log_error("no curseinfo function for %s and no fallback either.\n", c->type->cname);
        }
        else {
            log_error("no curseinfo function for %s, using cinfo_simple fallback.\n", c->type->cname);
        }
        return msg;
    }
}

const struct unit *ucansee(const struct faction *f, const struct unit *u,
    const struct unit *x)
{
    if (cansee(f, u->region, u, 0))
        return u;
    return x;
}

int stealth_modifier(int seen_mode)
{
    switch (seen_mode) {
    case see_unit:
        return 0;
    case see_far:
    case see_lighthouse:
        return -2;
    case see_travel:
        return -1;
    default:
        return INT_MIN;
    }
}

void transfer_seen(quicklist ** dst, quicklist ** src)
{
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

static void add_seen_faction_i(struct quicklist **flist, faction *f) {
    ql_set_insert_ex(flist, f, cmp_faction);
}

void add_seen_faction(faction *self, faction *seen) {
    add_seen_faction_i(&self->seen_factions, seen);
}

typedef struct address_data {
    faction *f, *lastf;
    quicklist **flist;
    int stealthmod;
} address_data;

static void cb_add_address(region *r, unit *ut, void *cbdata) {
    address_data *data = (address_data *)cbdata;
    faction *f = data->f;

    if (ut->faction==f) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            faction *sf = visible_faction(f, u);
            assert(u->faction != f);   /* if this is see_travel only, then I shouldn't be here. */
            if (data->lastf != sf && cansee_unit(u, ut, data->stealthmod)) {
                add_seen_faction_i(data->flist, sf);
                data->lastf = sf;
                break;
            }
        }
    }
}

static void add_travelthru_addresses(region *r, faction *f, quicklist **flist, int stealthmod) {
    // for each traveling unit: add the faction of any unit is can see
    address_data cbdata = { 0 };
    cbdata.f = f;
    cbdata.flist = flist;
    cbdata.stealthmod = stealthmod;
    travelthru_map(r, cb_add_address, &cbdata);
}

static void get_addresses(report_context * ctx)
{
    /* "TODO: travelthru" */
    seen_region *sr = NULL;
    region *r;
    const faction *lastf = NULL;
    quicklist *flist = 0;

    transfer_seen(&flist, &ctx->f->seen_factions);

    ctx->f->seen_factions = NULL; /* do not delete it twice */
    ql_push(&flist, ctx->f);

    if (f_get_alliance(ctx->f)) {
        quicklist *ql = ctx->f->alliance->members;
        int qi;
        for (qi = 0; ql; ql_advance(&ql, &qi, 1)) {
            add_seen_faction_i(&flist, (faction *)ql_get(ql, qi));
        }
    }

    /* find the first region that this faction can see */
    for (r = ctx->first; sr == NULL && r != ctx->last; r = r->next) {
        sr = find_seen(ctx->f->seen, r);
    }

    for (; sr != NULL; sr = sr->next) {
        int stealthmod = stealth_modifier(sr->mode);
        r = sr->r;
        if (sr->mode == see_lighthouse) {
            unit *u = r->units;
            for (; u; u = u->next) {
                faction *sf = visible_faction(ctx->f, u);
                if (lastf != sf) {
                    if (u->building || u->ship || (stealthmod > INT_MIN
                        && cansee(ctx->f, r, u, stealthmod))) {
                        add_seen_faction_i(&flist, sf);
                        lastf = sf;
                    }
                }
            }
        }
        else if (sr->mode == see_travel) {
            /* when we travel through a region, then we must add
             * the factions of any units we saw */
            add_travelthru_addresses(r, ctx->f, &flist, stealthmod);
        }
        else if (sr->mode > see_travel) {
            const unit *u = r->units;
            while (u != NULL) {
                if (u->faction != ctx->f) {
                    faction *sf = visible_faction(ctx->f, u);
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
    type->extension = extension;
    type->write = write;
    type->flag = flag;
    type->next = report_types;
    report_types = type;
}

static quicklist *get_regions_distance(region * root, int radius)
{
    quicklist *ql, *rlist = NULL;
    int qi = 0;

    ql_push(&rlist, root);
    fset(root, RF_MARK);
    ql = rlist;

    while (ql) {
        region *r = (region *)ql_get(ql, qi);
        region * next[MAXDIRECTIONS];
        int d;
        get_neighbours(r, next);

        for (d = 0; d != MAXDIRECTIONS; ++d) {
            if (next[d] && !fval(next[d], RF_MARK) && distance(next[d], root) <= radius) {
                ql_push(&rlist, next[d]);
                fset(next[d], RF_MARK);
            }
        }
        ql_advance(&ql, &qi, 1);
    }
    for (ql = rlist, qi = 0; ql; ql_advance(&ql, &qi, 1)) {
        region *r = (region *)ql_get(ql, qi);
        freset(r, RF_MARK);
    }
    return rlist;
}

void view_default(struct seen_region **seen, region * r, faction * f)
{
    int dir;
    for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
        region *r2 = rconnect(r, dir);
        if (r2) {
            connection *b = get_borders(r, r2);
            while (b) {
                if (!b->type->transparent(b, f))
                    break;
                b = b->next;
            }
            if (!b)
                add_seen(seen, r2, see_neighbour, false);
        }
    }
}

void view_neighbours(struct seen_region **seen, region * r, faction * f)
{
    int d;
    region * nb[MAXDIRECTIONS];

    get_neighbours(r, nb);
    for (d = 0; d != MAXDIRECTIONS; ++d) {
        region *r2 = nb[d];
        if (r2) {
            connection *b = get_borders(r, r2);
            while (b) {
                if (!b->type->transparent(b, f))
                    break;
                b = b->next;
            }
            if (!b) {
                if (add_seen(seen, r2, see_far, false)) {
                    if (!(fval(r2->terrain, FORBIDDEN_REGION))) {
                        int dir;
                        for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
                            region *r3 = rconnect(r2, dir);
                            if (r3) {
                                connection *b = get_borders(r2, r3);
                                while (b) {
                                    if (!b->type->transparent(b, f))
                                        break;
                                    b = b->next;
                                }
                                if (!b)
                                    add_seen(seen, r3, see_neighbour, false);
                            }
                        }
                    }
                }
            }
        }
    }
}

static void
recurse_regatta(struct seen_region **seen, region * center, region * r,
faction * f, int maxdist)
{
    int d;
    int dist = distance(center, r);
    region * nb[MAXDIRECTIONS];

    get_neighbours(r, nb);
    for (d = 0; d != MAXDIRECTIONS; ++d) {
        region *r2 = nb[d];
        if (r2) {
            int ndist = distance(center, r2);
            if (ndist > dist && fval(r2->terrain, SEA_REGION)) {
                connection *b = get_borders(r, r2);
                while (b) {
                    if (!b->type->transparent(b, f))
                        break;
                    b = b->next;
                }
                if (!b) {
                    if (ndist < maxdist) {
                        if (add_seen(seen, r2, see_far, false)) {
                            recurse_regatta(seen, center, r2, f, maxdist);
                        }
                    }
                    else
                        add_seen(seen, r2, see_neighbour, false);
                }
            }
        }
    }
}

static void view_regatta(struct seen_region **seen, region * r, faction * f)
{
    unit *u;
    int skill = 0;
    for (u = r->units; u; u = u->next) {
        if (u->faction == f) {
            int es = effskill(u, SK_PERCEPTION, 0);
            if (es > skill)
                skill = es;
        }
    }
    recurse_regatta(seen, r, r, f, skill / 2);
}

static void prepare_lighthouse(building * b, faction * f)
{
    int range = lighthouse_range(b, f);
    quicklist *ql, *rlist = get_regions_distance(b->region, range);
    int qi;

    for (ql = rlist, qi = 0; ql; ql_advance(&ql, &qi, 1)) {
        region *rl = (region *)ql_get(ql, qi);
        if (!fval(rl->terrain, FORBIDDEN_REGION)) {
            region * next[MAXDIRECTIONS];
            int d;

            get_neighbours(rl, next);
            faction_add_seen(f, rl, see_lighthouse);
            for (d = 0; d != MAXDIRECTIONS; ++d) {
                if (next[d]) {
                    faction_add_seen(f, next[d], see_neighbour);
                }
            }
        }
    }
    ql_free(rlist);
}

void reorder_units(region * r)
{
    unit **unext = &r->units;

    if (r->buildings) {
        building *b = r->buildings;
        while (*unext && b) {
            unit **ufirst = unext;    /* where the first unit in the building should go */
            unit **umove = unext;     /* a unit we consider moving */
            unit *owner = building_owner(b);
            while (owner && *umove) {
                unit *u = *umove;
                if (u->building == b) {
                    unit **uinsert = unext;
                    if (u == owner) {
                        uinsert = ufirst;
                    }
                    if (umove != uinsert) {
                        *umove = u->next;
                        u->next = *uinsert;
                        *uinsert = u;
                    }
                    else {
                        /* no need to move, skip ahead */
                        umove = &u->next;
                    }
                    if (unext == uinsert) {
                        /* we have a new well-placed unit. jump over it */
                        unext = &u->next;
                    }
                }
                else {
                    umove = &u->next;
                }
            }
            b = b->next;
        }
    }

    if (r->ships) {
        ship *sh = r->ships;
        /* first, move all units up that are not on ships */
        unit **umove = unext;       /* a unit we consider moving */
        while (*umove) {
            unit *u = *umove;
            if (u->number && !u->ship) {
                if (umove != unext) {
                    *umove = u->next;
                    u->next = *unext;
                    *unext = u;
                }
                else {
                    /* no need to move, skip ahead */
                    umove = &u->next;
                }
                /* we have a new well-placed unit. jump over it */
                unext = &u->next;
            }
            else {
                umove = &u->next;
            }
        }

        while (*unext && sh) {
            unit **ufirst = unext;    /* where the first unit in the building should go */
            unit **umove = unext;     /* a unit we consider moving */
            unit *owner = ship_owner(sh);
            while (owner && *umove) {
                unit *u = *umove;
                if (u->number && u->ship == sh) {
                    unit **uinsert = unext;
                    if (u == owner) {
                        uinsert = ufirst;
                        owner = u;
                    }
                    if (umove != uinsert) {
                        *umove = u->next;
                        u->next = *uinsert;
                        *uinsert = u;
                    }
                    else {
                        /* no need to move, skip ahead */
                        umove = &u->next;
                    }
                    if (unext == uinsert) {
                        /* we have a new well-placed unit. jump over it */
                        unext = &u->next;
                    }
                }
                else {
                    umove = &u->next;
                }
            }
            sh = sh->next;
        }
    }
}

static void cb_add_seen(region *r, unit *u, void *cbdata) {
    unused_arg(cbdata);
    if (u->faction) {
        faction_add_seen(u->faction, r, see_travel);
    }
}

static void prepare_reports(void)
{
    region *r;
    faction *f;
    building *b;
    const struct building_type *bt_lighthouse = bt_find("lighthouse");

    for (f = factions; f; f = f->next) {
        if (f->seen) seen_done(f->seen);
        f->seen = seen_init();
    }

    for (r = regions; r; r = r->next) {
        unit *u;

        reorder_units(r);

        /* Region owner get always the Lighthouse report */
        if (bt_lighthouse && config_token("rules.region_owner_pay_building", bt_lighthouse->_name)) {
            for (b = rbuildings(r); b; b = b->next) {
                if (b && b->type == bt_lighthouse) {
                    u = building_owner(b);
                    if (u) {
                        prepare_lighthouse(b, u->faction);
                        if (u_race(u) != get_race(RC_SPELL) || u->number == RS_FARVISION) {
                            seen_region *sr = faction_add_seen(u->faction, r, see_unit);
                            if (fval(u, UFL_DISBELIEVES)) {
                                sr->disbelieves = true;
                            }
                        }
                    }
                }
            }
        }

        for (u = r->units; u; u = u->next) {
            if (u->building && u->building->type == bt_lighthouse) {
                /* we are in a lighthouse. add the regions we can see from here! */
                prepare_lighthouse(u->building, u->faction);
            }

            if (u_race(u) != get_race(RC_SPELL) || u->number == RS_FARVISION) {
                seen_region *sr = faction_add_seen(u->faction, r, see_unit);
                if (fval(u, UFL_DISBELIEVES)) {
                    sr->disbelieves = true;
                }
            }
        }


        if (fval(r, RF_TRAVELUNIT)) {
            travelthru_map(r, cb_add_seen, r);
        }
    }
}

static void cb_set_last(region *r, unit *u, void *cbdata) {
    faction *f = (faction *)cbdata;
    if (u->faction == f) {
        f->last = r;
    }
}

static region *lastregion(faction * f)
{
#ifdef SMART_INTERVALS
    unit *u = f->units;
    region *r = f->last;

    if (u == NULL)
        return NULL;
    if (r != NULL)
        return r->next;

    /* it is safe to start in the region of the first unit. */
    f->last = u->region;
    /* if regions have indices, we can skip ahead: */
    for (u = u->nextF; u != NULL; u = u->nextF) {
        r = u->region;
        if (r->index > f->last->index)
            f->last = r;
    }

    /* we continue from the best region and look for travelthru etc. */
    for (r = f->last->next; r; r = r->next) {
        /* search the region for travelthru-attributes: */
        if (fval(r, RF_TRAVELUNIT)) {
            travelthru_map(r, cb_set_last, f);
        }
        if (f->last == r)
            continue;
        if (check_leuchtturm(r, f))
            f->last = r;
    }
    return f->last->next;
#else
    return NULL;
#endif
}

static region *firstregion(faction * f)
{
#ifdef SMART_INTERVALS
    region *r = f->first;

    if (f->units == NULL)
        return NULL;
    if (r != NULL)
        return r;

    return f->first = regions;
#else
    return regions;
#endif
}

static void cb_view_neighbours(seen_region *sr, void *cbdata) {
    faction *f = (faction *)cbdata;
    if (sr->mode > see_neighbour) {
        region *r = sr->r;
        plane *p = rplane(r);
        void(*view) (struct seen_region **, region *, faction *) = view_default;

        if (p && fval(p, PFL_SEESPECIAL)) {
            /* TODO: this is not very customizable */
            view = (strcmp(p->name, "Regatta") == 0) ? view_regatta : view_neighbours;
        }
        view(f->seen, r, f);
    }
}

void prepare_seen(faction *f)
{
    region *r;
    struct seen_region *sr;

    for (r = f->first, sr = NULL; sr == NULL && r != f->last; r = r->next) {
        sr = find_seen(f->seen, r);
    }

    seenhash_map(f->seen, cb_view_neighbours, f);
    get_seen_interval(f->seen, &f->first, &f->last);
    link_seen(f->seen, f->first, f->last);
}

static void prepare_report(struct report_context *ctx, faction *f)
{
    prepare_seen(f);
    ctx->f = f;
    ctx->report_time = time(NULL);
    ctx->addresses = NULL;
    ctx->userdata = NULL;
    ctx->first = firstregion(f);
    ctx->last = lastregion(f);
}

int write_reports(faction * f, time_t ltime)
{
    unsigned int backup = 1, maxbackup = 128 * 1000;
    bool gotit = false;
    struct report_context ctx;
    const char *encoding = "UTF-8";
    report_type *rtype;

    if (noreports) {
        return false;
    }
    prepare_report(&ctx, f);
    get_addresses(&ctx);
    log_debug("Reports for %s:", factionname(f));
    for (rtype = report_types; rtype != NULL; rtype = rtype->next) {
        if (f->options & rtype->flag) {
            int error;
            do {
                char filename[MAX_PATH];
                sprintf(filename, "%s/%d-%s.%s", reportpath(), turn, factionid(f),
                    rtype->extension);
                error = 0;
                if (rtype->write(filename, &ctx, encoding) == 0) {
                    gotit = true;
                }
                if (errno) {
                    char zText[64];
                    log_warning("retrying, error %d during %s report for faction %s", error, rtype->extension, factionname(f));
                    sprintf(zText, "waiting %u seconds before we retry", backup / 1000);
                    perror(zText);
                    _sleep(backup);
                    if (backup < maxbackup) {
                        backup *= 2;
                    }
                    error = errno;
                    errno = 0;
                }
            } while (error);
        }
    }
    if (!gotit) {
        log_warning("No report for faction %s!", factionid(f));
    }
    ql_free(ctx.addresses);
    if (ctx.f->seen) {
        seen_done(ctx.f->seen);
    }
    return 0;
}

static void write_script(FILE * F, const faction * f)
{
    report_type *rtype;
    char buf[1024];

    fprintf(F, "faction=%s:email=%s:lang=%s", factionid(f), f->email,
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
                strlcat(buf, ",", sizeof(buf));
            }
            strlcat(buf, rtype->extension, sizeof(buf));
        }
    }
    fputs(buf, F);
    fputc('\n', F);
}

static void check_messages_exist(void) {
    ct_checknames();
}

int init_reports(void)
{
    check_messages_exist();
    create_directories();
    prepare_reports();
    return 0;
}

int reports(void)
{
    faction *f;
    FILE *mailit;
    time_t ltime = time(NULL);
    int retval = 0;
    char path[MAX_PATH];
    const char * rpath = reportpath();

    log_info("Writing reports for turn %d:", turn);
    report_donations();
    remove_empty_units();

    join_path(rpath, "reports.txt", path, sizeof(path));
    mailit = fopen(path, "w");
    if (mailit == NULL) {
        log_error("%s could not be opened!\n", path);
    }

    for (f = factions; f; f = f->next) {
        int error = write_reports(f, ltime);
        if (error)
            retval = error;
        if (mailit)
            write_script(mailit, f);
    }
    if (mailit)
        fclose(mailit);
    free_seen();
#ifdef GLOBAL_REPORT
    {
        const char *str = config_get("globalreport");
        if (str != NULL) {
            sprintf(path, "%s/%s.%u.cr", reportpath(), str, turn);
            global_report(path);
        }
    }
#endif
    return retval;
}

static variant var_copy_string(variant x)
{
    x.v = x.v ? _strdup((const char *)x.v) : 0;
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

static void var_free_regions(variant x)
{
    free(x.v);
}

const char *trailinto(const region * r, const struct locale *lang)
{
    char ref[32];
    const char *s;
    if (r) {
        const char *tname = terrain_name(r);
        size_t sz;

        sz = strlcpy(ref, tname, sizeof(ref));
        sz += strlcat(ref+sz, "_trail", sizeof(ref)-sz);
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
        len = strlcpy(buffer, "(Chaos)", size);
    }
    else {
        plane *pl = rplane(r);
        const char *name = pl ? pl->name : 0;
        int nx = r->x, ny = r->y;
        int named = (name && name[0]);
        pnormalize(&nx, &ny, pl);
        adjust_coordinates(f, &nx, &ny, pl);
        len = strlcpy(buffer, rname(r, f ? f->locale : 0), size);
        _snprintf(buffer + len, size - len, " (%d,%d%s%s)", nx, ny, named ? "," : "", (named) ? name : "");
        buffer[size - 1] = 0;
        len = strlen(buffer);
    }
    return len;
}

static char *f_regionid_s(const region * r, const faction * f)
{
    static int i = 0;
    static char bufs[4][NAMESIZE + 20]; // FIXME: static return value
    char *buf = bufs[(++i) % 4];

    f_regionid(r, f, buf, NAMESIZE + 20);
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
        sp ? spell_name(sp, f->locale) : LOC(f->locale, "an_unknown_spell");
    assert(c || !"spell without description!");
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_curse(struct opstack **stack, const void *userdata)
{                               /* unit -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct curse_type *sp = (const struct curse_type *)opop(stack).v;
    const char *c =
        sp ? curse_name(sp, f->locale) : LOC(f->locale, "an_unknown_curse");
    assert(c || !"spell effect without description!");
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_unitname(struct opstack **stack, const void *userdata)
{                               /* unit -> string */
    const struct faction *f = (const struct faction *)userdata;
    const struct unit *u = (const struct unit *)opop(stack).v;
    const char *c = u ? unit_getname(u) : LOC(f->locale, "an_unknown_unit");
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
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

    var.i = u->number;
    opush(stack, var);
}

static void eval_faction(struct opstack **stack, const void *userdata)
{                               /* faction -> string */
    const struct faction *f = (const struct faction *)opop(stack).v;
    const char *c = factionname(f);
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_alliance(struct opstack **stack, const void *userdata)
{                               /* faction -> string */
    const struct alliance *al = (const struct alliance *)opop(stack).v;
    const char *c = alliancename(al);
    variant var;
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
            sprintf(buffer, "%u %s", weight / SCALEWEIGHT, LOC(lang,
                "weight_unit_p"));
        }
    }
    else {
        if (weight == 1) {
            sprintf(buffer, "1 %s %u", LOC(lang, "weight_per"), SCALEWEIGHT);
        }
        else {
            sprintf(buffer, "%u %s %u", weight, LOC(lang, "weight_per_p"),
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
    const char *c = LOC(lang, resourcename(res, j != 1));
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_race(struct opstack **stack, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const struct locale *lang = report ? report->locale : default_locale;
    int j = opop(stack).i;
    const race *r = (const race *)opop(stack).v;
    const char *c = LOC(lang, rc_name_s(r, (j == 1) ? NAME_SINGULAR : NAME_PLURAL));
    size_t len = strlen(c);
    variant var;

    var.v = strcpy(balloc(len + 1), c);
    opush(stack, var);
}

static void eval_order(struct opstack **stack, const void *userdata)
{                               /* order -> string */
    const struct order *ord = (const struct order *)opop(stack).v;
    char buf[4096];
    size_t len;
    variant var;

    unused_arg(userdata);
    write_order(ord, buf, sizeof(buf));
    len = strlen(buf);
    var.v = strcpy(balloc(len + 1), buf);
    opush(stack, var);
}

static void eval_resources(struct opstack **stack, const void *userdata)
{                               /* order -> string */
    const faction *report = (const faction *)userdata;
    const struct locale *lang = report ? report->locale : default_locale;
    const struct resource *res = (const struct resource *)opop(stack).v;
    char buf[1024];        /* but we only use about half of this */
    size_t size = sizeof(buf) - 1;
    variant var;

    char *bufp = buf;
    while (res != NULL && size > 4) {
        const char *rname =
            resourcename(res->type, (res->number != 1) ? NMF_PLURAL : 0);
        int result = _snprintf(bufp, size, "%d %s", res->number, LOC(lang, rname));
        if (wrptr(&bufp, &size, result) != 0 || size < sizeof(buf) / 2) {
            WARN_STATIC_BUFFER();
            break;
        }

        res = res->next;
        if (res != NULL && size > 2) {
            strcat(bufp, ", ");
            bufp += 2;
            size -= 2;
        }
    }
    *bufp = 0;
    var.v = strcpy(balloc((size_t)(bufp - buf + 1)), buf);
    opush(stack, var);
}

static void eval_regions(struct opstack **stack, const void *userdata)
{                               /* order -> string */
    const faction *report = (const faction *)userdata;
    int i = opop(stack).i;
    int end, begin = opop(stack).i;
    const arg_regions *regions = (const arg_regions *)opop(stack).v;
    char buf[256];
    size_t size = sizeof(buf) - 1;
    variant var;
    char *bufp = buf;

    if (regions == NULL) {
        end = begin;
    }
    else {
        if (i >= 0)
            end = begin + i;
        else
            end = regions->nregions + i;
    }
    for (i = begin; i < end; ++i) {
        const char *rname = (const char *)regionname(regions->regions[i], report);
        bufp = STRLCPY(bufp, rname, size);

        if (i + 1 < end && size > 2) {
            strcat(bufp, ", ");
            bufp += 2;
            size -= 2;
        }
    }
    *bufp = 0;
    var.v = strcpy(balloc((size_t)(bufp - buf + 1)), buf);
    opush(stack, var);
}

static void eval_trail(struct opstack **stack, const void *userdata)
{                               /* order -> string */
    const faction *report = (const faction *)userdata;
    const struct locale *lang = report ? report->locale : default_locale;
    int i, end = 0, begin = 0;
    const arg_regions *regions = (const arg_regions *)opop(stack).v;
    char buf[512];
    size_t size = sizeof(buf) - 1;
    variant var;
    char *bufp = buf;
#ifdef _SECURECRT_ERRCODE_VALUES_DEFINED
    /* stupid MS broke _snprintf */
    int eold = errno;
#endif

    if (regions != NULL) {
        end = regions->nregions;
        for (i = begin; i < end; ++i) {
            region *r = regions->regions[i];
            const char *trail = trailinto(r, lang);
            const char *rn = f_regionid_s(r, report);

            if (wrptr(&bufp, &size, _snprintf(bufp, size, trail, rn)) != 0)
                WARN_STATIC_BUFFER();

            if (i + 2 < end) {
                bufp = STRLCPY(bufp, ", ", size);
            }
            else if (i + 1 < end) {
                bufp = STRLCPY(bufp, LOC(lang, "list_and"), size);
            }
        }
    }
    *bufp = 0;
    var.v = strcpy(balloc((size_t)(bufp - buf + 1)), buf);
    opush(stack, var);
#ifdef _SECURECRT_ERRCODE_VALUES_DEFINED
    if (errno == ERANGE) {
        errno = eold;
    }
#endif
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
    unused_arg(userdata);
}

/*** END MESSAGE RENDERING ***/

/* - String Listen --------------------------------------------- */
void addstrlist(strlist ** SP, const char *s)
{
    strlist *slist = malloc(sizeof(strlist));
    slist->next = NULL;
    slist->s = _strdup(s);
    addlist(SP, slist);
}

void freestrlist(strlist * s)
{
    strlist *q, *p = s;
    while (p) {
        q = p->next;
        free(p->s);
        free(p);
        p = q;
    }
}

#include <util/nrmessage.h>

static void log_orders(const struct message *msg)
{
    char buffer[4096];
    int i;

    for (i = 0; i != msg->type->nparameters; ++i) {
        if (msg->type->types[i]->copy == &var_copy_order) {
            const char *section = nr_section(msg);
            nr_render(msg, default_locale, buffer, sizeof(buffer), NULL);
            log_debug("MESSAGE [%s]: %s\n", section, buffer);
            break;
        }
    }
}

int stream_printf(struct stream * out, const char *format, ...) {
    va_list args;
    int result;
    char buffer[4096];
    size_t bytes = sizeof(buffer);
    // TODO: should be in storage/stream.c (doesn't exist yet)
    va_start(args, format);
    result = vsnprintf(buffer, bytes, format, args);
    if (result >= 0 && (size_t)result < bytes) {
        bytes = (size_t)result;
        // TODO: else = buffer too small
    }
    out->api->write(out->handle, buffer, bytes);
    va_end(args);
    return result;
}

typedef struct count_data {
    int n;
    const struct faction *f;
} count_data;

static void count_cb(region *r, unit *u, void *cbdata) {
    count_data *data = (count_data *)cbdata;
    const struct faction *f = data->f;
    if (r != u->region && (!u->ship || ship_owner(u->ship) == u)) {
        if (cansee_durchgezogen(f, r, u, 0)) {
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
    register_argtype("regions", var_free_regions, NULL, VAR_VOIDPTR);

    msg_log_create = &log_orders;

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
    add_function("unit.name", &eval_unitname);
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

    /* register alternative visibility functions */
    register_function((pf_generic)view_neighbours, "view_neighbours");
    register_function((pf_generic)view_regatta, "view_regatta");
}
