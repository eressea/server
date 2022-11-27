#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "report.h"

#include <spells/regioncurse.h>
#include <spells/buildingcurse.h>

/* modules includes */
#include <modules/score.h>

/* attributes includes */
#include <attributes/otherfaction.h>
#include <attributes/reduceproduction.h>
#include <attributes/seenspell.h>

/* gamecode includes */
#include "alchemy.h"
#include "economy.h"
#include "guard.h"
#include "laws.h"
#include "market.h"
#include "monsters.h"
#include "move.h"
#include "recruit.h"
#include "reports.h"
#include "teleport.h"
#include "travelthru.h"
#include "upkeep.h"
#include "vortex.h"

/* kernel includes */
#include "kernel/alliance.h"
#include "kernel/ally.h"
#include "kernel/attrib.h"
#include "kernel/calendar.h"
#include "kernel/config.h"
#include "kernel/connection.h"
#include "kernel/build.h"
#include "kernel/building.h"
#include "kernel/curse.h"
#include "kernel/faction.h"
#include "kernel/group.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/objtypes.h"
#include "kernel/order.h"
#include "kernel/plane.h"
#include "kernel/pool.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/render.h"
#include "kernel/resources.h"
#include "kernel/ship.h"
#include "kernel/spell.h"
#include "kernel/spellbook.h"
#include "kernel/terrain.h"
#include "kernel/terrainid.h"
#include "kernel/unit.h"

/* util includes */
#include <util/base36.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/message.h>
#include <util/nrmessage.h>
#include "util/param.h"
#include <util/rng.h>
#include <util/strings.h>

#include <format.h>
#include <selist.h>
#include <filestream.h>
#include <stream.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>

#define TEMPLATE_BOM 1

/* pre-C99 compatibility */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)(-1))
#endif

#define ECHECK_VERSION "4.01"

extern int *storms;
extern int weeks_per_month;
extern int months_per_year;

static char *gamedate_season(const struct locale *lang)
{
    static char buf[256]; /* FIXME: static return value */
    gamedate gd;

    assert(weeknames);

    get_gamedate(turn, &gd);

    sprintf(buf, (const char *)LOC(lang, "nr_calendar_season"),
        LOC(lang, mkname("calendar", weeknames[gd.week])),
        LOC(lang, mkname("calendar", calendar_month(gd.month))),
        gd.year,
        LOC(lang, mkname("calendar", calendar_era())), LOC(lang, mkname("calendar", seasonnames[gd.season])));

    return buf;
}

void newline(struct stream *out) {
    sputs("", out);
}

void write_spaces(struct stream *out, size_t num) {
    static const char spaces[REPORTWIDTH] = "                                                                             ";
    while (num > 0) {
        size_t bytes = (num > REPORTWIDTH) ? REPORTWIDTH : num;
        swrite(spaces, sizeof(char), bytes, out);
        num -= bytes;
    }
}

static void centre(struct stream *out, const char *s, bool breaking)
{
    /* Bei Namen die genau 80 Zeichen lang sind, kann es hier Probleme
     * geben. Seltsamerweise wird i dann auf MAXINT oder aehnlich
     * initialisiert. Deswegen keine Strings die laenger als REPORTWIDTH
     * sind! */

    if (breaking && REPORTWIDTH < strlen(s)) {
        strlist *T, *SP = NULL;
        sparagraph(&SP, s, 0, 0);
        T = SP;
        while (SP) {
            centre(out, SP->s, false);
            SP = SP->next;
        }
        freestrlist(T);
    }
    else {
        write_spaces(out, (REPORTWIDTH - strlen(s) + 1) / 2);
        sputs(s, out);
    }
}

void paragraph(struct stream *out, const char *str, ptrdiff_t indent,
    int hanging_indent, char marker)
{
    size_t length = REPORTWIDTH;
    const char *handle_end, *begin, *mark = NULL;

    if (!str) return;
    /* find out if there's a mark + indent already encoded in the string. */
    if (!marker) {
        const char *x = str;
        while (*x == ' ')
            ++x;
        indent += x - str;
        if (x[0] && indent && x[1] == ' ') {
            indent += 2;
            mark = x;
            str = x + 2;
            hanging_indent -= 2;
        }
    }
    else {
        mark = &marker;
    }
    begin = handle_end = str;

    do {
        const char *last_space = begin;

        if (mark && indent >= 2) {
            write_spaces(out, indent - 2);
            swrite(mark, sizeof(char), 1, out);
            write_spaces(out, 1);
            mark = 0;
        }
        else if (begin == str) {
            write_spaces(out, indent);
        }
        else {
            write_spaces(out, indent + hanging_indent);
        }
        while (*handle_end && handle_end <= begin + length - indent) {
            if (*handle_end == ' ') {
                last_space = handle_end;
            }
            ++handle_end;
        }
        if (*handle_end == 0)
            last_space = handle_end;
        if (last_space == begin) {
            /* there was no space in this line. clip it */
            last_space = handle_end;
        }
        swrite(begin, sizeof(char), last_space - begin, out);
        begin = last_space;
        while (*begin == ' ') {
            ++begin;
        }
        if (begin > handle_end)
            begin = handle_end;
        sputs("", out);
    } while (*begin);
}

static bool write_spell_modifier(const spell * sp, int flag, const char * str, bool cont, sbstring *sbp) {
    if (sp->sptyp & flag) {
        if (cont) {
            sbs_strcat(sbp, ", ");
        }
        else {
            sbs_strcat(sbp, " ");
        }
        sbs_strcat(sbp, str);
        return true;
    }
    return cont;
}

void nr_spell_syntax(char *buf, size_t size, spellbook_entry * sbe, const struct locale *lang)
{
    const spell *sp = spellref_get(&sbe->spref);
    const char *params = sp->parameter;
    const char *spname;
    sbstring sbs;

    sbs_init(&sbs, buf, size);
    if (sp->sptyp & ISCOMBATSPELL) {
        sbs_strcat(&sbs, LOC(lang, keyword(K_COMBATSPELL)));
    }
    else {
        sbs_strcat(&sbs, LOC(lang, keyword(K_CAST)));
    }

    /* Reihenfolge beachten: Erst REGION, dann STUFE! */
    if (sp->sptyp & FARCASTING) {
        sbs_strcat(&sbs, " [");
        sbs_strcat(&sbs, LOC(lang, parameters[P_REGION]));
        sbs_strcat(&sbs, " x y]");
    }
    if (sp->sptyp & SPELLLEVEL) {
        sbs_strcat(&sbs, " [");
        sbs_strcat(&sbs, LOC(lang, parameters[P_LEVEL]));
        sbs_strcat(&sbs, " n]");
    }

    spname = spell_name(mkname_spell(sp), lang);
    if (strchr(spname, ' ') != NULL) {
        /* contains spaces, needs quotes */
        sbs_strcat(&sbs, " \"");
        sbs_strcat(&sbs, spname);
        sbs_strcat(&sbs, "\"");
    }
    else {
        sbs_strcat(&sbs, " ");
        sbs_strcat(&sbs, spname);
    }

    while (params && *params) {
        typedef struct starget {
            param_t param;
            int flag;
            const char *vars;
        } starget;
        starget targets[] = {
            { P_REGION, REGIONSPELL, NULL },
            { P_UNIT, UNITSPELL, "par_unit" },
            { P_SHIP, SHIPSPELL, "par_ship" },
            { P_BUILDING, BUILDINGSPELL, "par_building" },
            { 0, 0, NULL }
        };
        starget *targetp;
        char cp = *params++;
        const char *locp;
        const char *syntaxp = sp->syntax;

        if (cp == 'u') {
            targetp = targets + 1;
            locp = LOC(lang, targetp->vars);
            sbs_strcat(&sbs, " <");
            sbs_strcat(&sbs, locp);
            sbs_strcat(&sbs, ">");
            if (*params == '+') {
                ++params;
                sbs_strcat(&sbs, " [<");
                sbs_strcat(&sbs, locp);
                sbs_strcat(&sbs, "> ...]");
            }
        }
        else if (cp == 's') {
            targetp = targets + 2;
            locp = LOC(lang, targetp->vars);
            sbs_strcat(&sbs, " <");
            sbs_strcat(&sbs, locp);
            sbs_strcat(&sbs, ">");
            if (*params == '+') {
                ++params;
                sbs_strcat(&sbs, " [<");
                sbs_strcat(&sbs, locp);
                sbs_strcat(&sbs, "> ...]");
            }
        }
        else if (cp == 'r') {
            sbs_strcat(&sbs, " <x> <y>");
            if (*params == '+') {
                ++params;
                sbs_strcat(&sbs, " [<x> <y> ...]");
            }
        }
        else if (cp == 'b') {
            targetp = targets + 3;
            locp = LOC(lang, targetp->vars);
            sbs_strcat(&sbs, " <");
            sbs_strcat(&sbs, locp);
            sbs_strcat(&sbs, ">");
            if (*params == '+') {
                ++params;
                sbs_strcat(&sbs, " [<");
                sbs_strcat(&sbs, locp);
                sbs_strcat(&sbs, "> ...]");
            }
        }
        else if (cp == 'k') {
            int i, maxparam = 0;
            bool multi = false;
            if (params && *params == 'c') {
                /* skip over a potential id */
                ++params;
            }
            if (params && *params == '+') {
                ++params;
                multi = true;
            }
            for (targetp = targets; targetp->flag; ++targetp) {
                if (sp->sptyp & targetp->flag)
                    ++maxparam;
            }
            if (!maxparam || maxparam > 1) {
                sbs_strcat(&sbs, " (");
            }
            i = 0;
            for (targetp = targets; targetp->flag; ++targetp) {
                if (!maxparam || sp->sptyp & targetp->flag) {
                    if (i++ != 0) {
                        sbs_strcat(&sbs, " |");
                    }
                    if (targetp->param && targetp->vars) {
                        locp = LOC(lang, targetp->vars);
                        sbs_strcat(&sbs, " ");
                        sbs_strcat(&sbs, parameters[targetp->param]);
                        sbs_strcat(&sbs, " <");
                        sbs_strcat(&sbs, locp);
                        sbs_strcat(&sbs, ">");
                        if (multi) {
                            sbs_strcat(&sbs, " [<");
                            sbs_strcat(&sbs, locp);
                            sbs_strcat(&sbs, "> ...]");
                        }
                    }
                    else {
                        sbs_strcat(&sbs, " ");
                        sbs_strcat(&sbs, parameters[targetp->param]);
                    }
                }
            }
            if (!maxparam || maxparam > 1) {
                sbs_strcat(&sbs, " )");
            }
        }
        else if (cp == 'i' || cp == 'c') {
            const char *cstr;
            assert(syntaxp);
            cstr = strchr(syntaxp, ':');
            if (!cstr) {
                locp = LOC(lang, mkname("spellpar", syntaxp));
            }
            else {
                char substr[32];
                size_t len = cstr - syntaxp;
                assert(sizeof(substr) > len);
                memcpy(substr, syntaxp, len);
                substr[len] = 0;
                locp = LOC(lang, mkname("spellpar", substr));
                syntaxp = substr + 1;
            }
            if (*params == '?') {
                ++params;
                sbs_strcat(&sbs, " [<");
                sbs_strcat(&sbs, locp);
                sbs_strcat(&sbs, ">]");
            }
            else {
                sbs_strcat(&sbs, " <");
                sbs_strcat(&sbs, locp);
                sbs_strcat(&sbs, ">");
            }
        }
        else {
            log_error("unknown spell parameter %c for spell %s", cp, sp->sname);
        }
    }
}

void nr_spell(struct stream *out, spellbook_entry * sbe, const struct locale *lang)
{
    int k;
    bool cont;
    char buf[4096];
    const spell *sp = spellref_get(&sbe->spref);
    sbstring sbs;

    newline(out);
    centre(out, spell_name(mkname_spell(sp), lang), true);
    newline(out);
    paragraph(out, LOC(lang, "nr_spell_description"), 0, 0, 0);
    paragraph(out, spell_info(sp, lang), 2, 0, 0);

    sbs_init(&sbs, buf, sizeof(buf));
    sbs_strcat(&sbs, LOC(lang, "nr_spell_type"));
    sbs_strcat(&sbs, " ");

    if (sp->sptyp & PRECOMBATSPELL) {
        sbs_strcat(&sbs, LOC(lang, "sptype_precombat"));
    }
    else if (sp->sptyp & COMBATSPELL) {
        sbs_strcat(&sbs, LOC(lang, "sptype_combat"));
    }
    else if (sp->sptyp & POSTCOMBATSPELL) {
        sbs_strcat(&sbs, LOC(lang, "sptype_postcombat"));
    }
    else {
        sbs_strcat(&sbs, LOC(lang, "sptype_normal"));
    }
    paragraph(out, buf, 0, 0, 0);

    sprintf(buf, "%s %d", LOC(lang, "nr_spell_level"), sbe->level);
    paragraph(out, buf, 0, 0, 0);

    sprintf(buf, "%s %d", LOC(lang, "nr_spell_rank"), sp->rank);
    paragraph(out, buf, 0, 0, 0);

    paragraph(out, LOC(lang, "nr_spell_components"), 0, 0, 0);
    for (k = 0; sp->components[k].type; ++k) {
        const resource_type *rtype = sp->components[k].type;
        int itemanz = sp->components[k].amount;
        int costtyp = sp->components[k].cost;
        if (itemanz > 0) {
            sbs_init(&sbs, buf, sizeof(buf));
            if (sp->sptyp & SPELLLEVEL) {
                sbs_strcat(&sbs, "  ");
                sbs_strcat(&sbs, str_itoa(itemanz));
                sbs_strcat(&sbs, " ");
                sbs_strcat(&sbs, LOC(lang, resourcename(rtype, itemanz != 1)));

                if (costtyp == SPC_LEVEL || costtyp == SPC_LINEAR) {
                    sbs_strcat(&sbs, "  * ");
                    sbs_strcat(&sbs, LOC(lang, "nr_level"));
                }
            }
            else {
                sbs_strcat(&sbs, str_itoa(itemanz));
                sbs_strcat(&sbs, " ");
                sbs_strcat(&sbs, LOC(lang, resourcename(rtype, itemanz != 1)));
            }
            paragraph(out, buf, 2, 2, '-');
        }
    }

    sbs_substr(&sbs, 0, 0);
    sbs_strcat(&sbs, LOC(lang, "nr_spell_modifiers"));

    cont = false;
    cont = write_spell_modifier(sp, FARCASTING, LOC(lang, "smod_far"), cont, &sbs);
    cont = write_spell_modifier(sp, OCEANCASTABLE, LOC(lang, "smod_sea"), cont, &sbs);
    cont = write_spell_modifier(sp, ONSHIPCAST, LOC(lang, "smod_ship"), cont, &sbs);
    cont = write_spell_modifier(sp, NOTFAMILIARCAST, LOC(lang, "smod_nofamiliar"), cont, &sbs);
    if (!cont) {
        write_spell_modifier(sp, NOTFAMILIARCAST, LOC(lang, "smod_none"), cont, &sbs);
    }
    paragraph(out, buf, 0, 0, 0);
    paragraph(out, LOC(lang, "nr_spell_syntax"), 0, 0, 0);

    nr_spell_syntax(buf, sizeof(buf), sbe, lang);
    paragraph(out, buf, 2, 0, 0);

    newline(out);
}

static void
nr_curses_i(struct stream *out, int indent, const faction *viewer, objtype_t typ, const void *obj, attrib *a, int self)
{
    for (; a; a = a->next) {
        message *msg = NULL;

        if (a->type == &at_curse) {
            curse *c = (curse *)a->data.v;

            self = curse_cansee(c, viewer, typ, obj, self);
            msg = msg_curse(c, obj, typ, self);
        }
        else if (a->type == &at_effect && self) {
            int value = effect_value(a);
            if (value > 0) {
                const struct item_type* itype = effect_type(a);
                msg = msg_message("nr_potion_effect", "potion left",
                    itype->rtype, value);
            }
        }
        if (msg) {
            char buf[4096];
            newline(out);
            nr_render(msg, viewer->locale, buf, sizeof(buf), viewer);
            paragraph(out, buf, indent, 2, 0);
            msg_release(msg);
        }
    }
}

static void nr_curses(struct stream *out, int indent, const faction *viewer, objtype_t typ, const void *obj)
{
    int self = 0;
    attrib *a = NULL;
    region *r;

    /* Die Sichtbarkeit eines Zaubers und die Zaubermeldung sind bei
    * Gebaeuden und Schiffen je nach, ob man Besitzer ist, verschieden.
    * Bei Einheiten sieht man Wirkungen auf eigene Einheiten immer.
    * Spezialfaelle (besonderes Talent, verursachender Magier usw. werde
    * bei jedem curse gesondert behandelt. */
    if (typ == TYP_SHIP) {
        ship *sh = (ship *)obj;
        unit *owner = ship_owner(sh);
        a = sh->attribs;
        r = sh->region;
        if (owner) {
            if (owner->faction == viewer) {
                self = 2;
            }
            else {                  /* steht eine person der Partei auf dem Schiff? */
                unit *u = NULL;
                for (u = r->units; u; u = u->next) {
                    if (u->ship == sh) {
                        self = 1;
                        break;
                    }
                }
            }
        }
    }
    else if (typ == TYP_BUILDING) {
        building *b = (building *)obj;
        unit *owner;
        a = b->attribs;
        r = b->region;
        if ((owner = building_owner(b)) != NULL) {
            if (owner->faction == viewer) {
                self = 2;
            }
            else {                  /* steht eine Person der Partei in der Burg? */
                unit *u = NULL;
                for (u = r->units; u; u = u->next) {
                    if (u->building == b) {
                        self = 1;
                        break;
                    }
                }
            }
        }
    }
    else if (typ == TYP_UNIT) {
        unit *u = (unit *)obj;
        a = u->attribs;
        r = u->region;
        if (u->faction == viewer) {
            self = 2;
        }
    }
    else if (typ == TYP_REGION) {
        r = (region *)obj;
        a = r->attribs;
    }
    else {
        log_error("get_attribs: invalid object type %d", typ);
        assert(!"invalid object type");
    }
    nr_curses_i(out, indent, viewer, typ, obj, a, self);
}

static void rps_nowrap(struct stream *out, const char *s)
{
    const char *x = s;
    size_t indent = 0;

    if (!x || !*x) return;

    while (*x++ == ' ');
    indent = x - s - 1;
    if (*(x - 1) && indent && *x == ' ')
        indent += 2;
    x = s;
    while (*s) {
        if (s == x) {
            x = strchr(x + 1, ' ');
            if (!x)
                x = s + strlen(s);
        }
        swrite(s++, sizeof(char), 1, out);
    }
}

static void
nr_unit(struct stream *out, const faction * f, const unit * u, int indent, seen_mode mode)
{
    char bullet;
    bool isbattle = (mode == seen_battle);
    char buf[8192];
    faction* of = get_otherfaction(u);
    bool anon = fval(u, UFL_ANON_FACTION);

    newline(out);
    bufunit_depr(f, u, mode, buf, sizeof(buf));

    bullet = bufunit_bullet(f, u, of, anon);
    paragraph(out, buf, indent, 0, bullet);

    if (!isbattle) {
        nr_curses(out, indent, f, TYP_UNIT, u);
    }
}

static void
rp_messages(struct stream *out, message_list * msgs, faction * viewer, int indent,
    bool categorized)
{
    int i;
    if (!msgs) {
        return;
    }
    for (i = 0; i != MAXSECTIONS && sections[i]; ++i) {
        const char * section = sections[i];
        int k = 0;
        struct mlist *m = msgs->begin;
        while (m) {
            /* categorized messages need a section: */
            assert(!categorized || (m->msg->type->section != NULL));
            if (!categorized || m->msg->type->section == section) {
                char lbuf[8192];

                if (!k && categorized) {
                    const char *section_title;
                    char cat_identifier[24];

                    newline(out);
                    sprintf(cat_identifier, "section_%s", section);
                    section_title = LOC(viewer->locale, cat_identifier);
                    if (section_title) {
                        centre(out, section_title, true);
                        newline(out);
                    }
                    else {
                        log_error("no title defined for section %s in locale %s", section, locale_name(viewer->locale));
                    }
                    k = 1;
                }
                nr_render(m->msg, viewer->locale, lbuf, sizeof(lbuf), viewer);
                /* Hack: some messages should start a new paragraph with a newline: */
                if (strncmp("para_", m->msg->type->name, 5) == 0) {
                    newline(out);
                }
                paragraph(out, lbuf, indent, 2, 0);
            }
            m = m->next;
        }
        if (!categorized)
            break;
    }
}

static void rp_battles(struct stream *out, faction * f)
{
    if (f->battles != NULL) {
        struct bmsg *bm = f->battles;
        newline(out);
        centre(out, LOC(f->locale, "section_battle"), false);
        newline(out);

        while (bm) {
            char buf[256];
            RENDER(f, buf, sizeof(buf), ("header_battle", "region", bm->r));
            centre(out, buf, true);
            newline(out);
            rp_messages(out, bm->msgs, f, 0, false);
            bm = bm->next;
            newline(out);
        }
    }
}

static void append_message(sbstring *sbp, message *m, const faction * f) {
    /* FIXME: this is a bad hack, cause by how msg_message works. */
    size_t size = sbp->size - sbs_length(sbp);
    size = nr_render(m, f->locale, sbp->end, size, f);
    sbp->end += size;
}

static void report_prices(struct stream *out, const region * r, const faction * f)
{
    const luxury_type *sale = NULL;
    struct demand *dmd;
    message *m;
    int n = 0;
    char buf[4096];
    sbstring sbs;

    if (r->land == NULL || r->land->demands == NULL)
        return;
    for (dmd = r->land->demands; dmd; dmd = dmd->next) {
        if (dmd->value == 0)
            sale = dmd->type;
        else if (dmd->value > 0)
            n++;
    }
    assert(sale != NULL);

    m = msg_message("nr_market_sale", "product price",
        sale->itype->rtype, sale->price);
    newline(out);
    nr_render(m, f->locale, buf, sizeof(buf), f);
    msg_release(m);
    sbs_adopt(&sbs, buf, sizeof(buf));

    if (n > 0) {
        sbs_strcat(&sbs, " ");
        sbs_strcat(&sbs, LOC(f->locale, "nr_trade_intro"));
        sbs_strcat(&sbs, " ");

        for (dmd = r->land->demands; dmd; dmd = dmd->next) {
            if (dmd->value > 0) {
                m = msg_message("nr_market_price", "product price",
                    dmd->type->itype->rtype, dmd->value * dmd->type->price);
                append_message(&sbs, m, f);
                msg_release(m);
                n--;
                if (n == 0) {
                    sbs_strcat(&sbs, LOC(f->locale, "nr_trade_end"));
                }
                else if (n == 1) {
                    sbs_strcat(&sbs, " ");
                    sbs_strcat(&sbs, LOC(f->locale, "nr_trade_final"));
                    sbs_strcat(&sbs, " ");
                }
                else {
                    sbs_strcat(&sbs, LOC(f->locale, "nr_trade_next"));
                    sbs_strcat(&sbs, " ");
                }
            }
        }
    }
    paragraph(out, buf, 0, 0, 0);
}

bool see_border(const connection * b, const faction * f, const region * r)
{
    bool cs = b->type->fvisible(b, f, r);
    if (!cs) {
        cs = b->type->rvisible(b, r);
        if (!cs) {
            const unit *us = r->units;
            while (us && !cs) {
                if (us->faction == f) {
                    cs = b->type->uvisible(b, us);
                    if (cs)
                        break;
                }
                us = us->next;
            }
        }
    }
    return cs;
}

#define MAX_EDGES 16

struct edge {
    char *name;
    bool transparent;
    bool block;
    bool exist[MAXDIRECTIONS];
    direction_t lastd;
};

static void report_region_resource(sbstring *sbp, const struct locale *lang, const struct resource_type *rtype, int n) {
    if (n != 0) {
        sbs_strcat(sbp, ", ");
        sbs_strcat(sbp, str_itoa(n));
        sbs_strcat(sbp, " ");
        sbs_strcat(sbp, LOC(lang, resourcename(rtype, (n != 1) ? GR_PLURAL : 0)));
    }
}

static void report_roads(sbstring *sbs, const region *r, const faction* f, const bool see[])
{
    attrib* a;
    int d, nrd = 0;
    bool dh = false;

    /* Nachbarregionen, die gesehen werden, ermitteln */
    for (d = 0; d != MAXDIRECTIONS; d++) {
        if (see[d] && rconnect(r, d))
            nrd++;
    }

    /* list directions */
    for (d = 0; d != MAXDIRECTIONS; d++) {
        if (see[d]) {
            region* r2 = rconnect(r, d);
            if (!r2)
                continue;
            nrd--;
            if (dh) {
                char regname[128], trail[256];
                if (nrd == 0) {
                    sbs_strcat(sbs, " ");
                    sbs_strcat(sbs, LOC(f->locale, "nr_nb_final"));
                }
                else {
                    sbs_strcat(sbs, LOC(f->locale, "nr_nb_next"));
                }
                sbs_strcat(sbs, LOC(f->locale, directions[d]));
                sbs_strcat(sbs, " ");
                f_regionid(r2, f, regname, sizeof(regname));
                snprintf(trail, sizeof(trail), trailinto(r2, f->locale), regname);
                sbs_strcat(sbs, trail);
            }
            else {
                message* msg = msg_message("nr_vicinitystart", "dir region", d, r2);
                sbs_strcat(sbs, " ");
                append_message(sbs, msg, f);
                msg_release(msg);
                dh = true;
            }
        }
    }
    if (dh) {
        sbs_strcat(sbs, ".");
    }
    /* Spezielle Richtungen */
    for (a = a_find(r->attribs, &at_direction); a && a->type == &at_direction;
        a = a->next) {
        spec_direction* spd = (spec_direction*)(a->data.v);
        sbs_strcat(sbs, " ");
        sbs_strcat(sbs, LOC(f->locale, spd->desc));
        sbs_strcat(sbs, " (\"");
        sbs_strcat(sbs, LOC(f->locale, spd->keyword));
        sbs_strcat(sbs, "\").");
    }
}

static void report_region_description(struct stream *out, const region * r, faction * f, const bool see[])
{
    int n;
    int trees;
    int saplings;
    const char *tname;
    char buf[4096];
    sbstring sbs;

    f_regionid(r, f, buf, sizeof(buf));
    sbs_adopt(&sbs, buf, sizeof(buf));

    if (r->seen.mode == seen_travel) {
        sbs_strcat(&sbs, " (");
        sbs_strcat(&sbs, LOC(f->locale, "see_travel"));
        sbs_strcat(&sbs, ")");
    }
    else if (r->seen.mode == seen_neighbour) {
        sbs_strcat(&sbs, " (");
        sbs_strcat(&sbs, LOC(f->locale, "see_neighbour"));
        sbs_strcat(&sbs, ")");
    }
    else if ((r->seen.mode == seen_lighthouse)
        || (r->seen.mode == seen_lighthouse_land)) {
        sbs_strcat(&sbs, " (");
        sbs_strcat(&sbs, LOC(f->locale, "see_lighthouse"));
        sbs_strcat(&sbs, ")");
    }

    /* Terrain */
    tname = terrain_name(r);
    sbs_strcat(&sbs, ", ");
    sbs_strcat(&sbs, LOC(f->locale, tname));
    pump_paragraph(&sbs, out, REPORTWIDTH, false);

    /* Trees */
    trees = rtrees(r, 2);
    saplings = rtrees(r, 1);
    if (max_production(r)) {
        if (trees > 0 || saplings > 0) {
            sbs_strcat(&sbs, ", ");
            sbs_strcat(&sbs, str_itoa(trees));
            sbs_strcat(&sbs, "/");
            sbs_strcat(&sbs, str_itoa(saplings));
            sbs_strcat(&sbs, " ");

            if (fval(r, RF_MALLORN)) {
                if (trees == 1) {
                    sbs_strcat(&sbs, LOC(f->locale, "nr_mallorntree"));
                }
                else {
                    sbs_strcat(&sbs, LOC(f->locale, "nr_mallorntree_p"));
                }
            }
            else if (trees == 1) {
                sbs_strcat(&sbs, LOC(f->locale, "tree"));
            }
            else {
                sbs_strcat(&sbs, LOC(f->locale, "tree_p"));
            }
        }
    }
    pump_paragraph(&sbs, out, REPORTWIDTH, false);

    /* iron & stone */
    if (r->seen.mode >= seen_unit) {
        resource_report result[MAX_RAWMATERIALS];
        int numresults = report_resources(r, result, f, r->seen.mode);

        for (n = 0; n < numresults; ++n) {
            if (result[n].number >= 0 && result[n].level >= 0) {
                const char * name = resourcename(result[n].rtype, result[n].number != 1);
                sbs_strcat(&sbs, ", ");
                sbs_strcat(&sbs, str_itoa(result[n].number));
                sbs_strcat(&sbs, " ");
                sbs_strcat(&sbs, LOC(f->locale, name));
                sbs_strcat(&sbs, "/");
                sbs_strcat(&sbs, str_itoa(result[n].level));
            }
        }
    }

    /* peasants & silver */
    n = rpeasants(r);
    if (n) {
        sbs_strcat(&sbs, ", ");
        sbs_strcat(&sbs, str_itoa(n));

        if (r->land->ownership) {
            const char *str =
                LOC(f->locale, mkname("morale", itoa10(region_get_morale(r))));
            sbs_strcat(&sbs, " ");
            sbs_strcat(&sbs, str);
        }
        sbs_strcat(&sbs, " ");
        sbs_strcat(&sbs, LOC(f->locale, n != 1 ? "peasant_p" : "peasant"));

        if (is_mourning(r, turn + 1)) {
            sbs_strcat(&sbs, " ");
            sbs_strcat(&sbs, LOC(f->locale, "nr_mourning"));
        }
    }
    if (r->seen.mode >= seen_travel) {
        report_region_resource(&sbs, f->locale, get_resourcetype(R_SILVER), rmoney(r));
        /* Pferde */
        report_region_resource(&sbs, f->locale, get_resourcetype(R_HORSE), rhorses(r));
    }
    sbs_strcat(&sbs, ".");

    if (r->land && r->land->display && r->land->display[0]) {
        sbs_strcat(&sbs, " ");
        sbs_strcat(&sbs, r->land->display);

        n = r->land->display[strlen(r->land->display) - 1];
        if (n != '!' && n != '?' && n != '.') {
            sbs_strcat(&sbs, ".");
        }
    }

    if (rule_region_owners()) {
        const faction *owner = region_get_owner(r);

        if (owner != NULL) {
            message *msg;

            sbs_strcat(&sbs, " ");
            msg = msg_message("nr_region_owner", "faction", owner);
            append_message(&sbs, msg, f);
            msg_release(msg);
        }
    }

    pump_paragraph(&sbs, out, REPORTWIDTH, false);

    report_roads(&sbs, r, f, see);
    pump_paragraph(&sbs, out, REPORTWIDTH, true);
}

/**
 * Show roads and certain magic effects.
 */
static void report_region_edges(struct stream *out, const region * r, faction * f, struct edge edges[], int nedges) {
    nr_curses(out, 0, f, TYP_REGION, r);

    if (nedges > 0) {
        int e;
        newline(out);
        for (e = 0; e != nedges; ++e) {
            message *msg;
            int d;

            for (d = 0; d != MAXDIRECTIONS; ++d) {
                if (edges[e].exist[d]) {
                    char buf[512];
                    msg = msg_message(edges[e].transparent ? "nr_border_transparent" : "nr_border_opaque",
                        "object dir", edges[e].name, d);
                    nr_render(msg, f->locale, buf, sizeof(buf), f);
                    msg_release(msg);
                    paragraph(out, buf, 0, 0, 0);
                }
            }
            free(edges[e].name);
        }
    }
}

char *report_list(const struct locale *lang, char *buffer, size_t len, int argc, const char **argv) {
    const char *two = LOC(lang, "list_two");
    const char *start = LOC(lang, "list_start");
    const char *middle = LOC(lang, "list_middle");
    const char *end = LOC(lang, "list_end");
    return format_list(argc, argv, buffer, len, two, start, middle, end);
}

static void report_region_schemes(struct stream *out, const region * r, faction * f) {

    /* Sonderbehandlung Teleport-Ebene */
    region *rl[MAX_SCHEMES];
    int num = get_astralregions(r, inhabitable, rl);
    char buf[4096];

    if (num == 1) {
        /* single region is easy */
        region *rn = rl[0];
        f_regionid(rn, f, buf, sizeof(buf));
    }
    else if (num > 1) {
        int i;
        const char *rnames[MAX_SCHEMES];

        for (i = 0; i != num; ++i) {
            char rbuf[REPORTWIDTH];
            region *rn = rl[i];
            f_regionid(rn, f, rbuf, sizeof(rbuf));
            rnames[i] = str_strdup(rbuf);
        }
        report_list(f->locale, buf, sizeof(buf), num, rnames);
        for (i = 0; i != num; ++i) {
            free((char *)rnames[i]);
        }
    }
    if (num > 0) {
        if (format_replace(LOC(f->locale, "nr_schemes_template"), "{0}", buf, buf, sizeof(buf))) {
            newline(out);
            paragraph(out, buf, 0, 0, 0);
        }
    }
}

void report_region(struct stream *out, const region * r, faction * f)
{
    int d, ne = 0;
    bool see[MAXDIRECTIONS];
    struct edge edges[MAX_EDGES];

    assert(out);
    assert(f);
    assert(r);

    memset(edges, 0, sizeof(edges));
    for (d = 0; d != MAXDIRECTIONS; d++) {
        /* Nachbarregionen, die gesehen werden, ermitteln */
        region *r2 = rconnect(r, d);
        connection *b;
        see[d] = true;
        if (!r2)
            continue;
        for (b = get_borders(r, r2); b;) {
            int e;
            struct edge *match = NULL;
            bool transparent = b->type->transparent(b, f);
            const char *name = border_name(b, r, f, GF_DETAILED | GF_ARTICLE);

            if (!transparent) {
                see[d] = false;
            }
            if (!see_border(b, f, r)) {
                b = b->next;
                continue;
            }
            for (e = 0; e != ne; ++e) {
                struct edge *edg = edges + e;
                if (edg->transparent == transparent && 0 == strcmp(name, edg->name)) {
                    match = edg;
                    break;
                }
            }
            if (match == NULL) {
                match = edges + ne;
                match->name = str_strdup(name);
                match->transparent = transparent;
                ++ne;
                assert(ne < MAX_EDGES);
            }
            match->lastd = d;
            match->exist[d] = true;
            b = b->next;
        }
    }

    report_region_description(out, r, f, see);
    if (see_schemes(r)) {
        report_region_schemes(out, r, f);
    }
    report_region_edges(out, r, f, edges, ne);
}

static void report_statistics(struct stream *out, const region * r, const faction * f)
{
    int p = rpeasants(r);
    message *m;
    char buf[4096];

    /* print */
    newline(out);
    m = msg_message("nr_stat_header", "region", r);
    nr_render(m, f->locale, buf, sizeof(buf), f);
    msg_release(m);
    paragraph(out, buf, 0, 0, 0);
    newline(out);

    /* Region */
    if (skill_enabled(SK_ENTERTAINMENT) && fval(r->terrain, LAND_REGION)
        && rmoney(r)) {
        m = msg_message("nr_stat_maxentertainment", "max", entertainmoney(r));
        nr_render(m, f->locale, buf, sizeof(buf), f);
        paragraph(out, buf, 2, 2, 0);
        msg_release(m);
    }
    if (max_production(r) && (!fval(r->terrain, SEA_REGION)
        || f->race == get_race(RC_AQUARIAN))) {
        if (markets_module()) {     /* hack */
            bool mourn = is_mourning(r, turn);
            int p_wage = peasant_wage(r, mourn);
            m =
                msg_message("nr_stat_salary_new", "max", p_wage);
        }
        else {
            m = msg_message("nr_stat_salary", "max", wage(r, f->race));
        }
        nr_render(m, f->locale, buf, sizeof(buf), f);
        paragraph(out, buf, 2, 2, 0);
        msg_release(m);
    }

    if (p) {
        m = msg_message("nr_stat_recruits", "max", max_recruits(r));
        nr_render(m, f->locale, buf, sizeof(buf), f);
        paragraph(out, buf, 2, 2, 0);
        msg_release(m);

        if (r->land->ownership) {
            m = msg_message("nr_stat_morale", "morale", region_get_morale(r));
            nr_render(m, f->locale, buf, sizeof(buf), f);
            paragraph(out, buf, 2, 2, 0);
            msg_release(m);
        }

    }

    /* info about units */
    if (r->seen.mode >= seen_unit) {
        int number;
        item *itm, *items = NULL;
        unit *u;

        if (!markets_module()) {
            if (buildingtype_exists(r, bt_find("caravan"), true)) {
                p *= 2;
            }
            if (p >= TRADE_FRACTION) {
                m = msg_message("nr_stat_luxuries", "max", p / TRADE_FRACTION);
                nr_render(m, f->locale, buf, sizeof(buf), f);
                paragraph(out, buf, 2, 2, 0);
                msg_release(m);
            }
        }

        /* count */
        for (number = 0, u = r->units; u; u = u->next) {
            if (u->faction == f) {
                for (itm = u->items; itm; itm = itm->next) {
                    i_change(&items, itm->type, itm->number);
                }
                number += u->number;
            }
        }
        m = msg_message("nr_stat_people", "max", number);
        nr_render(m, f->locale, buf, sizeof(buf), f);
        paragraph(out, buf, 2, 2, 0);
        msg_release(m);

        for (itm = items; itm; itm = itm->next) {
            sprintf(buf, "%s: %d",
                LOC(f->locale, resourcename(itm->type->rtype, GR_PLURAL)), itm->number);
            paragraph(out, buf, 2, 2, 0);
        }
        i_freeall(&items);
    }
}


static int buildingmaintenance(const building * b, const resource_type * rtype)
{
    const building_type *bt = b->type;
    int c, cost = 0;

    for (c = 0; bt->maintenance && bt->maintenance[c].number; ++c) {
        const maintenance *m = bt->maintenance + c;
        if (m->rtype == rtype) {
            if (fval(m, MTF_VARIABLE))
                cost += (b->size * m->number);
            else
                cost += m->number;
        }
    }
    return cost;
}

static int
report_template(const char *filename, report_context * ctx, const char *bom)
{
    return write_template(filename, TEMPLATE_BOM ? bom : NULL, ctx->f, ctx->password ? ctx->password : "password", ctx->first, ctx->last);
}

int write_template(const char* filename, const char* bom, const faction* f, const char* password,
    const region* regions_begin,
    const region* regions_end)
{
    const resource_type* rsilver = get_resourcetype(R_SILVER);
    const struct locale* lang = f->locale;
    const region* r;
    FILE* F = fopen(filename, "w");
    stream strm = { 0 }, * out = &strm;
    char buf[4096];
    sbstring sbs;

    if (F == NULL) {
        perror(filename);
        return -1;
    }
    fstream_init(&strm, F);

    if (TEMPLATE_BOM && bom) {
        swrite(bom, 1, strlen(bom), out);
    }

    sprintf(buf, "; %s\n", LOC(lang, "nr_template"));
    rps_nowrap(out, buf);

    if (!password) {
        sprintf(buf, "; %s\n", LOC(lang, "template_password_notice"));
        rps_nowrap(out, buf);
    }
    sprintf(buf, "%s %s \"%s\"", LOC(lang, parameters[P_FACTION]), itoa36(f->no), password);
    rps_nowrap(out, buf);
    newline(out);
    newline(out);
    sprintf(buf, "; ECHECK -l -w4 -r%d -v%s", f->race->recruitcost,
        ECHECK_VERSION);
    rps_nowrap(out, buf);
    newline(out);

    for (r = regions_begin; r != regions_end; r = r->next) {
        unit* u;
        int dh = 0;

        if (r->seen.mode < seen_unit)
            continue;

        for (u = r->units; u; u = u->next) {
            if (u->faction == f) {
                const order* ord;
                if (!dh) {
                    plane* pl = getplane(r);
                    int nx = r->x, ny = r->y;

                    pnormalize(&nx, &ny, pl);
                    adjust_coordinates(f, &nx, &ny, pl);
                    newline(out);
                    if (pl && pl->id != 0) {
                        sprintf(buf, "%s %d,%d,%d ; %s", LOC(lang,
                            parameters[P_REGION]), nx, ny, pl->id, rname(r, lang));
                    }
                    else {
                        sprintf(buf, "%s %d,%d ; %s", LOC(lang, parameters[P_REGION]),
                            nx, ny, rname(r, lang));
                    }
                    rps_nowrap(out, buf);
                    newline(out);
                    sprintf(buf, "; ECheck Lohn %d", wage(r, f->race));
                    rps_nowrap(out, buf);
                    newline(out);
                    newline(out);
                }
                dh = 1;

                sbs_init(&sbs, buf, sizeof(buf));
                sbs_strcat(&sbs, LOC(u->faction->locale, parameters[P_UNIT]));
                sbs_strcat(&sbs, " ");
                sbs_strcat(&sbs, itoa36(u->no)),
                sbs_strcat(&sbs, ";    ");
                sbs_strcat(&sbs, unit_getname(u));
                sbs_strcat(&sbs, " [");
                sbs_strcat(&sbs, str_itoa(u->number));
                sbs_strcat(&sbs, ",");
                sbs_strcat(&sbs, str_itoa(get_money(u)));
                sbs_strcat(&sbs, "$");
                if (u->building && building_owner(u->building) == u) {
                    building* b = u->building;
                    if (!curse_active(get_curse(b->attribs, &ct_nocostbuilding))) {
                        int cost = buildingmaintenance(b, rsilver);
                        if (cost > 0) {
                            sbs_strcat(&sbs, ",U");
                            sbs_strcat(&sbs, str_itoa(cost));
                        }
                    }
                }
                else if (u->ship) {
                    if (ship_owner(u->ship) == u) {
                        sbs_strcat(&sbs, ",S");
                    }
                    else {
                        sbs_strcat(&sbs, ",s");
                    }
                    sbs_strcat(&sbs, itoa36(u->ship->no));
                }
                if (lifestyle(u) == 0) {
                    sbs_strcat(&sbs, ",I");
                }
                sbs_strcat(&sbs, "]");
                rps_nowrap(out, buf);
                newline(out);

                for (ord = u->orders; ord; ord = ord->next) {
                    strcpy(buf, "   ");
                    write_order(ord, lang, buf + 2, sizeof(buf) - 2);
                    rps_nowrap(out, buf);
                    newline(out);
                }

                /* If the lastorder begins with an @ it should have
                 * been printed in the loop before. */
            }
        }
    }
    newline(out);
    str_strlcpy(buf, LOC(lang, parameters[P_NEXT]), sizeof(buf));
    rps_nowrap(out, buf);
    newline(out);
    fstream_done(&strm);
    return 0;
}

static int count_allies_cb(struct allies *all, faction *af, int status, void *udata) {
    int *num = (int *)udata;
    if (status > 0) {
        ++*num;
    }
    return 0;
}

struct show_s {
    sbstring sbs;
    stream *out;
    const faction *f;
    int num_allies;
    int num_listed;
    size_t maxlen;
};

/* TODO: does not test for non-ascii unicode spaces. */
#define IS_UTF8_SPACE(pos) (*pos > 0 && *pos <= CHAR_MAX && isspace(*pos))

void pump_paragraph(sbstring *sbp, stream *out, size_t maxlen, bool isfinal)
{
    while (sbs_length(sbp) > maxlen) {
        char *pos, *begin = sbp->begin;
        assert(begin);
        while (*begin && IS_UTF8_SPACE(begin)) {
            /* eat whitespace */
            ++begin;
        }
        pos = begin;
        do {
            char *next = strchr(pos+1, ' ');
            if (next == NULL) {
                if (isfinal) {
                    swrite(begin, 1, pos - begin, out);
                    while (*pos && IS_UTF8_SPACE(pos)) {
                        ++pos;
                    }
                    newline(out);
                    swrite(pos, 1, sbp->end - pos, out);
                    newline(out);
                }
                return;
            }
            else if (next > begin + maxlen) {
                ptrdiff_t len;
                if (pos == begin) {
                    /* there was no space before the break, line will be too long */
                    pos = next;
                }
                len = pos - begin;
                swrite(begin, 1, len, out);
                newline(out);

                while (*pos && IS_UTF8_SPACE(pos)) {
                    ++pos;
                }
                sbs_substr(sbp, pos - begin, SIZE_MAX);
                break;
            }
            pos = next;
        } while (pos);
    }
    if (isfinal) {
        char *pos = sbp->begin;
        while (*pos && IS_UTF8_SPACE(pos)) {
            /* eat whitespace */
            ++pos;
        }
        swrite(pos, 1, sbp->end - pos, out);
        newline(out);
    }
}

static int show_allies_cb(struct allies *all, faction *af, int status, void *udata) {
    struct show_s * show = (struct show_s *)udata;
    const faction * f = show->f;
    sbstring *sbp = &show->sbs;
    int mode = alliance_status(f, af, status);

    if (show->num_listed++ != 0) {
        if (show->num_listed == show->num_allies) {
            /* last entry */
            sbs_strcat(sbp, LOC(f->locale, "list_and"));
        }
        else {
            /* neither first entry nor last*/
            sbs_strcat(sbp, ", ");
        }
    }
    sbs_strcat(sbp, factionname(af));
    pump_paragraph(sbp, show->out, show->maxlen, false);
    sbs_strcat(sbp, " (");
    if ((mode & HELP_ALL) == HELP_ALL) {
        sbs_strcat(sbp, LOC(f->locale, parameters[P_ANY]));
    }
    else if (mode > 0) {
        int h, hh = 0;
        for (h = 1; h <= HELP_TRAVEL; h *= 2) {
            int p = MAXPARAMS;
            if ((mode & h) == h) {
                switch (h) {
                case HELP_TRAVEL:
                    p = P_TRAVEL;
                    break;
                case HELP_MONEY:
                    p = P_MONEY;
                    break;
                case HELP_FIGHT:
                    p = P_FIGHT;
                    break;
                case HELP_GIVE:
                    p = P_GIVE;
                    break;
                case HELP_GUARD:
                    p = P_GUARD;
                    break;
                case HELP_FSTEALTH:
                    p = P_FACTIONSTEALTH;
                    break;
                }
            }
            if (p != MAXPARAMS) {
                if (hh) {
                    sbs_strcat(sbp, ", ");
                }
                sbs_strcat(sbp, LOC(f->locale, parameters[p]));
                hh = 1;
            }
        }
    }
    if (show->num_allies == show->num_listed) {
        sbs_strcat(sbp, ").\n");
        pump_paragraph(sbp, show->out, show->maxlen, true);
    }
    else {
        sbs_strcat(sbp, ")");
        pump_paragraph(sbp, show->out, show->maxlen, false);
    }
    return 0;
}

void report_allies(struct stream *out, size_t maxlen, const struct faction * f, struct allies * allies, const char *prefix)
{
    int num_allies = 0;

    assert(maxlen <= REPORTWIDTH);
    allies_walk(allies, count_allies_cb, &num_allies);

    if (num_allies > 0) {
        struct show_s show;
        char buf[REPORTWIDTH * 2];
        show.f = f;
        show.out = out;
        show.num_allies = num_allies;
        show.num_listed = 0;
        show.maxlen = maxlen;
        sbs_init(&show.sbs, buf, sizeof(buf));
        sbs_strcat(&show.sbs, prefix);

        allies_walk(allies, show_allies_cb, &show);
    }
}

static void rpline(struct stream *out)
{
    static char line[REPORTWIDTH + 1];
    if (line[0] != '-') {
        memset(line, '-', sizeof(line));
        line[REPORTWIDTH] = '\n';
    }
    swrite(line, sizeof(line), 1, out);
}

static void allies(struct stream *out, const faction * f)
{
    const group *g = f->groups;
    char prefix[64];

    rpline(out);
    newline(out);
    centre(out, LOC(f->locale, "nr_alliances"), false);
    newline(out);

    if (f->allies) {
        snprintf(prefix, sizeof(prefix), "%s ", LOC(f->locale, "faction_help"));
        report_allies(out, REPORTWIDTH, f, f->allies, prefix);
    }

    while (g) {
        if (g->allies) {
            snprintf(prefix, sizeof(prefix), "%s %s ", g->name, LOC(f->locale, "group_help"));
            report_allies(out, REPORTWIDTH, f, g->allies, prefix);
        }
        g = g->next;
    }
}

static void report_guards(struct stream *out, const region * r, const faction * see)
{
    /* die Partei  see  sieht dies; wegen
     * "unbekannte Partei", wenn man es selbst ist... */
    faction *guardians[512];
    int nextguard = 0;
    unit *u;
    int i;
    bool anonymous = false;
    /* Bewachung */

    for (u = r->units; u; u = u->next) {
        if (is_guard(u) != 0) {
            faction *f = u->faction;
            faction *fv = visible_faction(see, u, get_otherfaction(u));

            if (fv != f && see != fv) {
                f = fv;
            }

            if (f != see && fval(u, UFL_ANON_FACTION)) {
                anonymous = true;
            }
            else {
                for (i = 0; i != nextguard; ++i)
                    if (guardians[i] == f)
                        break;
                if (i == nextguard) {
                    guardians[nextguard++] = f;
                }
            }
        }
    }

    if (nextguard || anonymous) {
        char buf[2048];
        sbstring sbs;

        sbs_init(&sbs, buf, sizeof(buf));
        sbs_strcat(&sbs, LOC(see->locale, "nr_guarding_prefix"));

        newline(out);
        for (i = 0; i != nextguard + (anonymous ? 1 : 0); ++i) {
            if (i != 0) {
                if (i == nextguard - (anonymous ? 0 : 1)) {
                    sbs_strcat(&sbs, LOC(see->locale, "list_and"));
                }
                else {
                    sbs_strcat(&sbs, ", ");
                }
            }
            if (i < nextguard) {
                sbs_strcat(&sbs, factionname(guardians[i]));
            }
            else {
                sbs_strcat(&sbs, LOC(see->locale, "nr_guarding_unknown"));
            }
            pump_paragraph(&sbs, out, REPORTWIDTH, false);
        }
        sbs_strcat(&sbs, LOC(see->locale, "nr_guarding_postfix"));
        pump_paragraph(&sbs, out, REPORTWIDTH, true);
    }
}

static void list_address(struct stream *out, const faction * uf, selist * seenfactions)
{
    int qi = 0;
    selist *flist = seenfactions;

    rpline(out);
    newline(out);
    centre(out, LOC(uf->locale, "nr_addresses"), false);
    newline(out);

    while (flist != NULL) {
        const faction *f = (const faction *)selist_get(flist, qi);
        if (!is_monsters(f)) {
            const char *str;
            char buf[8192];
            char label = '-';

            str = faction_getbanner(f);
            sprintf(buf, "%s: %s; %s", factionname(f), faction_getemail(f),
                str ? str : "");
            if (uf == f)
                label = '*';
            else if (is_allied(uf, f)) {
                label = 'o';
            }
            else if (alliedfaction(uf, f, HELP_ALL))
                label = '+';
            paragraph(out, buf, 4, 0, label);
        }
        selist_advance(&flist, &qi, 1);
    }
}

static void
nr_ship(struct stream *out, const region *r, const ship * sh, const faction * f,
    const unit * captain)
{
    char buffer[1024];
    char ch;
    sbstring sbs;
    const char *stname;

    sbs_init(&sbs, buffer, sizeof(buffer));
    newline(out);

    stname = locale_plural(f->locale, sh->type->_name, sh->number, true);
    if (captain && captain->faction == f) {
        int n = 0, p = 0;

        getshipweight(sh, &n, &p);
        n = (n + 99) / 100;         /* 1 Silber = 1 GE */

        sbs_printf(&sbs, "%s, %d %s, (%d/%d)", shipname(sh), sh->number,
            stname, n, ship_capacity(sh) / 100);
    }
    else {
        sbs_printf(&sbs, "%s, %d %s", shipname(sh), sh->number, stname);
    }

    if (!ship_finished(sh)) {
        sbs_printf(&sbs, ", %s (%d/%d)",
            LOC(f->locale, "nr_undercons"), sh->size,
            ship_maxsize(sh));
    }
    if (sh->damage) {
        int percent = ship_damage_percent(sh);
        sbs_printf(&sbs, ", %d%% %s", percent, LOC(f->locale, "nr_damaged"));
    }
    if (!fval(r->terrain, SEA_REGION)) {
        if (sh->coast != NODIRECTION) {
            sbs_strcat(&sbs, ", ");
            sbs_strcat(&sbs, LOC(f->locale, coasts[sh->coast]));
        }
    }
    ch = 0;
    if (sh->display && sh->display[0]) {
        sbs_strcat(&sbs, "; ");
        sbs_strcat(&sbs, sh->display);
        ch = sh->display[strlen(sh->display) - 1];
    }
    if (ch != '!' && ch != '?' && ch != '.') {
        sbs_strcat(&sbs, ".");
    }
    paragraph(out, buffer, 2, 0, 0);

    nr_curses(out, 4, f, TYP_SHIP, sh);
}

static void
nr_building(struct stream *out, const region *r, const building *b, const faction *f)
{
    int i;
    const char *name, *bname, *billusion = NULL;
    const struct locale *lang;
    char buffer[8192];
    size_t size;
    sbstring sbs;

    assert(f);
    lang = f->locale;
    newline(out);
    sbs_init(&sbs, buffer, sizeof(buffer));

    report_building(b, &bname, &billusion);

    size = str_slprintf(buffer, sizeof(buffer), "%s, %s %d, ", buildingname(b),
        LOC(lang, "nr_size"), b->size);
    sbs.end += size;
    name = LOC(lang, billusion ? billusion : bname);
    sbs_strcat(&sbs, name);

    if (billusion) {
        unit *owner = building_owner(b);
        if (owner && owner->faction == f) {
            /* illusion. report real type */
            sbs_strcat(&sbs, " (");
            sbs_strcat(&sbs, LOC(lang, bname));
            sbs_strcat(&sbs, ")");
        }
    }

    if (!building_finished(b)) {
        sbs_strcat(&sbs, " ");
        sbs_strcat(&sbs, LOC(lang, "nr_building_inprogress"));
    }

    i = 0;
    if (b->display && b->display[0]) {
        sbs_strcat(&sbs, "; ");
        sbs_strcat(&sbs, b->display);
        i = b->display[strlen(b->display) - 1];
    }

    if (i != '!' && i != '?' && i != '.') {
        sbs_strcat(&sbs, ".");
    }
    paragraph(out, buffer, 2, 0, 0);

    nr_curses(out, 4, f, TYP_BUILDING, b);
}

static void nr_paragraph(struct stream *out, message * m, const faction * f)
{
    char buf[4096];

    assert(f);
    nr_render(m, f->locale, buf, sizeof(buf), f);
    msg_release(m);

    paragraph(out, buf, 0, 0, 0);
}

typedef struct travelthru_data {
    struct stream *out;
    char *handle_start, *writep;
    size_t size;
    const faction *f;
    int maxtravel, counter;
} travelthru_data;

static void init_cb(travelthru_data *data, struct stream *out, char *buffer, size_t size, const faction *f) {
    data->out = out;
    data->writep = buffer;
    data->handle_start = buffer;
    data->size = size;
    data->f = f;
    data->maxtravel = 0;
    data->counter = 0;
}

static void cb_write_travelthru(region *r, unit *u, void *cbdata) {
    travelthru_data *data = (travelthru_data *)cbdata;
    const faction *f = data->f;

    if (data->counter >= data->maxtravel) {
        return;
    }
    if (travelthru_cansee(r, f, u)) {
        ++data->counter;
        do {
            size_t len, size = data->size - (data->writep - data->handle_start);
            const char *str;
            char *writep = data->writep;

            if (u->ship != NULL) {
                str = shipname(u->ship);
            }
            else {
                str = unitname(u);
            }
            len = strlen(str);
            if (len < size && data->counter <= data->maxtravel) {
                memcpy(writep, str, len);
                writep += len;
                size -= len;
                if (data->counter == data->maxtravel) {
                    str = ".";
                }
                else if (data->counter + 1 == data->maxtravel) {
                    str = LOC(f->locale, "list_and");
                }
                else {
                    str = ", ";
                }
                len = str ? strlen(str) : 0;
                if (len < size) {
                    memcpy(writep, str, len);
                    writep += len;
                    size -= len;
                    data->writep = writep;
                }
            }
            if (len >= size || data->counter == data->maxtravel) {
                /* buffer is full */
                *writep = 0;
                paragraph(data->out, data->handle_start, 0, 0, 0);
                data->writep = data->handle_start;
                if (data->counter == data->maxtravel) {
                    break;
                }
            }
        } while (data->writep == data->handle_start);
    }
}

void report_travelthru(struct stream *out, region *r, const faction *f)
{
    assert(r);
    assert(f);
    if (fval(r, RF_TRAVELUNIT)) {
        int maxtravel = count_travelthru(r, f);

        if (maxtravel > 0) {
            travelthru_data cbdata;
            char buf[256];
            size_t bytes;

            newline(out);
            init_cb(&cbdata, out, buf, sizeof(buf), f);
            cbdata.maxtravel = maxtravel;
            bytes = str_strlcpy(buf, LOC(f->locale, "travelthru_header"), sizeof(buf));
            assert(bytes < sizeof(buf));
            cbdata.writep += bytes;
            travelthru_map(r, cb_write_travelthru, &cbdata);
            return;
        }
    }
}

static void report_market(stream * out, const region *r, const faction *f) {
    const item_type *lux = r_luxury(r);
    const item_type *herb = r->land->herbtype;
    message * m = NULL;

    if (herb && lux) {
        m = msg_message("nr_market_info_p", "p1 p2",
            lux->rtype, herb->rtype);
    }
    else if (lux) {
        m = msg_message("nr_market_info_s", "p1", lux->rtype);
    }
    else if (herb) {
        m = msg_message("nr_market_info_s", "p1", herb->rtype);
    }
    if (m) {
        newline(out);
        nr_paragraph(out, m, f);
    }
}

int
report_plaintext(const char *filename, report_context * ctx,
    const char *bom)
{
    int age, flag = 0;
    char ch;
    int anyunits, no_units, no_people;
    region *r;
    faction *f = ctx->f;
    unit *u;
    char pzTime[64];
    attrib *a;
    message *m;
    unsigned char op;
    int maxh, ix = WANT_OPTION(O_STATISTICS);
    int wants_stats = (f->options & ix);
    FILE *F = fopen(filename, "w");
    stream strm = { 0 }, *out = &strm;
    char buf[1024];
    sbstring sbs;

    if (F == NULL) {
        perror(filename);
        return -1;
    }
    fstream_init(&strm, F);

    if (bom) {
        fwrite(bom, 1, strlen(bom), F);
    }

    strftime(pzTime, 64, "%A, %d. %B %Y, %H:%M", localtime(&ctx->report_time));
    m = msg_message("nr_header_date", "game date", game_name(), pzTime);
    nr_render(m, f->locale, buf, sizeof(buf), f);
    msg_release(m);
    centre(out, buf, true);

    centre(out, gamedate_season(f->locale), true);
    newline(out);
    sprintf(buf, "%s, %s/%s (%s)", factionname(f),
        LOC(f->locale, rc_name_s(f->race, NAME_PLURAL)),
        magic_name(f->magiegebiet, f->locale), faction_getemail(f));
    centre(out, buf, true);
    if (f_get_alliance(f)) {
        centre(out, alliancename(f->alliance), true);
    }

    if (faction_age(f) <= 2) {
        const char *email;
        email = config_get("game.email");
        if (!email)
          log_error("game.email not set");
        else {
            const char *subject = get_mailcmd(f->locale);

            m = msg_message("newbie_info_game", "email subject", email, subject);
            if (m) {
                nr_render(m, f->locale, buf, sizeof(buf), f);
                msg_release(m);
                centre(out, buf, true);
            }
        }
        if ((f->options & WANT_OPTION(O_COMPUTER)) == 0) {
            const char *s;
            s = locale_getstring(f->locale, "newbie_info_cr");
            if (s) {
                newline(out);
                centre(out, s, true);
            }
            f->options |= WANT_OPTION(O_COMPUTER);
        }
    }
    newline(out);
    age = faction_age(f);
    if (f->options & WANT_OPTION(O_SCORE) && age > DISPLAYSCORE) {
        char score[32], avg[32];
        write_score(score, sizeof(score), f->score);
        write_score(avg, sizeof(avg), average_score_of_age(age));
        RENDER(f, buf, sizeof(buf), ("nr_score", "score average", score, avg));
        centre(out, buf, true);
    }
    no_units = f->num_units;
    no_people = f->num_people;
    m = msg_message("nr_population", "population units limit", no_people, no_units, rule_faction_limit());
    nr_render(m, f->locale, buf, sizeof(buf), f);
    msg_release(m);
    centre(out, buf, true);
    if (f->race == get_race(RC_HUMAN)) {
        int maxmig = count_maxmigrants(f);
        if (maxmig > 0) {
            m =
                msg_message("nr_migrants", "units maxunits", count_migrants(f), maxmig);
            nr_render(m, f->locale, buf, sizeof(buf), f);
            msg_release(m);
            centre(out, buf, true);
        }
    }
    if (f_get_alliance(f)) {
        m =
            msg_message("nr_alliance", "leader name id age",
                alliance_get_leader(f->alliance), f->alliance->name, f->alliance->id,
                turn - f->alliance_joindate);
        nr_render(m, f->locale, buf, sizeof(buf), f);
        msg_release(m);
        centre(out, buf, true);
    }
    maxh = max_heroes(f->num_people);
    if (maxh) {
        message *msg =
            msg_message("nr_heroes", "units maxunits", countheroes(f), maxh);
        nr_render(msg, f->locale, buf, sizeof(buf), f);
        msg_release(msg);
        centre(out, buf, true);
    }

    if (f->items != NULL) {
        message *msg = msg_message("nr_claims", "items", f->items);
        nr_render(msg, f->locale, buf, sizeof(buf), f);
        msg_release(msg);
        newline(out);
        centre(out, buf, true);
    }

    sbs_init(&sbs, buf, sizeof(buf));
    sbs_strcat(&sbs, LOC(f->locale, "nr_options"));
    sbs_strcat(&sbs, ":");
    for (op = 0; op != MAXOPTIONS; op++) {
        if (f->options & WANT_OPTION(op) && options[op]) {
            sbs_strcat(&sbs, " ");
            sbs_strcat(&sbs, LOC(f->locale, options[op]));

            ++flag;
        }
    }
    if (flag > 0) {
        newline(out);
        centre(out, buf, true);
    }

    rp_messages(out, f->msgs, f, 0, true);
    rp_battles(out, f);
    a = a_find(f->attribs, &at_reportspell);
    if (a) {
        newline(out);
        centre(out, LOC(f->locale, "section_newspells"), true);
        while (a && a->type == &at_reportspell) {
            spellbook_entry *sbe = (spellbook_entry *)a->data.v;
            nr_spell(out, sbe, f->locale);
            a = a->next;
        }
    }

    ch = 0;
    ERRNO_CHECK();
    for (a = a_find(f->attribs, &at_showitem); a && a->type == &at_showitem;
        a = a->next) {
        const item_type *itype = (const item_type *)a->data.v;
        if (itype) {
            const char *pname = resourcename(itype->rtype, 0);
            const char *description;

            if (ch == 0) {
                newline(out);
                centre(out, LOC(f->locale, "section_newpotions"), true);
                ch = 1;
            }

            newline(out);
            centre(out, LOC(f->locale, pname), true);
            snprintf(buf, sizeof(buf), "%s %d", LOC(f->locale, "nr_level"),
                potion_level(itype));
            centre(out, buf, true);
            newline(out);

            sbs_init(&sbs, buf, sizeof(buf));
            sbs_strcat(&sbs, LOC(f->locale, "nr_herbsrequired"));
            sbs_strcat(&sbs, ":");

            if (itype->construction) {
                requirement *rm = itype->construction->materials;
                while (rm->number) {
                    sbs_strcat(&sbs, LOC(f->locale, resourcename(rm->rtype, 0)));
                    ++rm;
                    if (rm->number) {
                        sbs_strcat(&sbs, ", ");
                    }
                }
            }
            centre(out, buf, true);
            newline(out);
            description = LOC(f->locale, mkname("describe", pname));
            centre(out, description, true);
        }
    }
    newline(out);
    ERRNO_CHECK();
    anyunits = 0;

    for (r = ctx->first; r != ctx->last; r = r->next) {
        int stealthmod = stealth_modifier(r, f, r->seen.mode);
        ship *sh = r->ships;

        if (r->seen.mode >= seen_lighthouse_land) {
            rpline(out);
            newline(out);
            report_region(out, r, f);
        }

        if (r->seen.mode >= seen_unit) {
            anyunits = 1;
            if (markets_module() && r->land) {
                report_market(out, r, f);
            }
            else if (!fval(r->terrain, SEA_REGION) && rpeasants(r) / TRADE_FRACTION > 0) {
                report_prices(out, r, f);
            }
            report_guards(out, r, f);
            report_travelthru(out, r, f);
            if (wants_stats) {
                report_statistics(out, r, f);
            }
        }
        else if (r->seen.mode >= seen_lighthouse) {
            report_travelthru(out, r, f);
        }

        /* Nachrichten an REGION in der Region */
        if (r->seen.mode >= seen_lighthouse) {
            message_list *mlist = r_getmessages(r, f);
            if (mlist) {
                struct mlist **split = merge_messages(mlist, r->msgs);
                newline(out);
                rp_messages(out, mlist, f, 0, false);
                split_messages(mlist, split);
            }
            else if (r->msgs) {
                newline(out);
                rp_messages(out, r->msgs, f, 0, false);
            }

            /* report all units. they are pre-sorted in an efficient manner */
            u = r->units;
            if (r->seen.mode >= seen_travel) {
                building *b = r->buildings;
                while (b) {
                    while (b && (!u || u->building != b)) {
                        nr_building(out, r, b, f);
                        b = b->next;
                    }
                    if (b) {
                        nr_building(out, r, b, f);
                        while (u && u->building == b) {
                            if (visible_unit(u, f, stealthmod, r->seen.mode)) {
                                nr_unit(out, f, u, 6, r->seen.mode);
                            }
                            u = u->next;
                        }
                        b = b->next;
                    }
                }
            }
            else while (u && u->building) {
                /* do not report units in buildings */
                u = u->next;
            }
            while (u && !u->ship) {
                if (visible_unit(u, f, stealthmod, r->seen.mode)) {
                    nr_unit(out, f, u, 4, r->seen.mode);
                }
                assert(!u->building);
                u = u->next;
            }
            while (sh) {
                while (sh && (!u || u->ship != sh)) {
                    nr_ship(out, r, sh, f, NULL);
                    sh = sh->next;
                }
                if (sh) {
                    nr_ship(out, r, sh, f, u);
                    while (u && u->ship == sh) {
                        if (visible_unit(u, f, stealthmod, r->seen.mode)) {
                            nr_unit(out, f, u, 6, r->seen.mode);
                        }
                        u = u->next;
                    }
                    sh = sh->next;
                }
            }
            assert(!u);
        }
        ERRNO_CHECK();
    }
    if (!is_monsters(f)) {
        if (!anyunits) {
            newline(out);
            paragraph(out, LOC(f->locale, "nr_youaredead"), 0, 2, 0);
        }
        else {
            allies(out, f);
            list_address(out, f, ctx->addresses);
        }
    }
    fstream_done(&strm);
    ERRNO_CHECK();
    return 0;
}

#define FMAXHASH 1021

struct fsee {
    struct fsee *nexthash;
    faction *f;
    struct see {
        struct see *next;
        faction *seen;
        unit *proof;
    } *see;
} *fsee[FMAXHASH];

#define REPORT_NR (1 << O_REPORT)
#define REPORT_CR (1 << O_COMPUTER)
#define REPORT_ZV (1 << O_ZUGVORLAGE)
#define REPORT_ZIP (1 << O_COMPRESS)
#define REPORT_BZIP2 (1 << O_BZIP2)

void register_nr(void)
{
    if (!nocr)
        register_reporttype("nr", &report_plaintext, 1 << O_REPORT);
    if (!nonr)
        register_reporttype("txt", &report_template, 1 << O_ZUGVORLAGE);
}

void report_cleanup(void)
{
    int i;
    for (i = 0; i != FMAXHASH; ++i) {
        while (fsee[i]) {
            struct fsee *fs = fsee[i]->nexthash;
            free(fsee[i]);
            fsee[i] = fs;
        }
    }
}
