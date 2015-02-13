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

#define ECHECK_VERSION "4.01"

#include <platform.h>
#include <kernel/config.h>

#include "reports.h"
#include "laws.h"
#include "monster.h"

/* modules includes */
#include <modules/score.h>

/* attributes includes */
#include <attributes/overrideroads.h>
#include <attributes/otherfaction.h>
#include <attributes/reduceproduction.h>

/* gamecode includes */
#include "alchemy.h"
#include "economy.h"
#include "move.h"
#include "upkeep.h"
#include "vortex.h"

/* kernel includes */
#include <kernel/ally.h>
#include <kernel/connection.h>
#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/objtypes.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/render.h>
#include <kernel/resources.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/teleport.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>
#include <kernel/alliance.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/message.h>
#include <util/nrmessage.h>
#include <quicklist.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>

extern int verbosity;
extern int *storms;
extern int weeks_per_month;
extern int months_per_year;

static char *gamedate_season(const struct locale *lang)
{
    static char buf[256]; // FIXME: static return value
    gamedate gd;

    get_gamedate(turn, &gd);

    sprintf(buf, (const char *)LOC(lang, "nr_calendar_season"),
        LOC(lang, weeknames[gd.week]),
        LOC(lang, monthnames[gd.month]),
        gd.year,
        agename ? LOC(lang, agename) : "", LOC(lang, seasonnames[gd.season]));

    return buf;
}

void rpc(FILE * F, char c, size_t num)
{
    while (num > 0) {
        putc(c, F);
        num--;
    }
}

void rnl(FILE * F)
{
    fputc('\n', F);
}

static void centre(FILE * F, const char *s, bool breaking)
{
    /* Bei Namen die genau 80 Zeichen lang sind, kann es hier Probleme
     * geben. Seltsamerweise wird i dann auf MAXINT oder aehnlich
     * initialisiert. Deswegen keine Strings die laenger als REPORTWIDTH
     * sind! */

    if (breaking && REPORTWIDTH < strlen(s)) {
        strlist *T, *SP = 0;
        sparagraph(&SP, s, 0, 0);
        T = SP;
        while (SP) {
            centre(F, SP->s, false);
            SP = SP->next;
        }
        freestrlist(T);
    }
    else {
        rpc(F, ' ', (REPORTWIDTH - strlen(s) + 1) / 2);
        fputs(s, F);
        putc('\n', F);
    }
}

static void
rparagraph(FILE * F, const char *str, ptrdiff_t indent, int hanging_indent,
char mark)
{
    static const char *spaces = "                                ";
    size_t length = REPORTWIDTH;
    const char *end, *begin;

    if (!str) return;
    /* find out if there's a mark + indent already encoded in the string. */
    if (!mark) {
        const char *x = str;
        while (*x == ' ')
            ++x;
        indent += x - str;
        if (x[0] && indent && x[1] == ' ') {
            indent += 2;
            mark = x[0];
            str = x + 2;
            hanging_indent -= 2;
        }
    }
    begin = end = str;

    do {
        const char *last_space = begin;

        if (mark && indent >= 2) {
            fwrite(spaces, sizeof(char), indent - 2, F);
            fputc(mark, F);
            fputc(' ', F);
            mark = 0;
        }
        else if (begin == str) {
            fwrite(spaces, sizeof(char), indent, F);
        }
        else {
            fwrite(spaces, sizeof(char), indent + hanging_indent, F);
        }
        while (*end && end <= begin + length - indent) {
            if (*end == ' ') {
                last_space = end;
            }
            ++end;
        }
        if (*end == 0)
            last_space = end;
        if (last_space == begin) {
            /* there was no space in this line. clip it */
            last_space = end;
        }
        fwrite(begin, sizeof(char), last_space - begin, F);
        begin = last_space;
        while (*begin == ' ') {
            ++begin;
        }
        if (begin > end)
            begin = end;
        fputc('\n', F);
    } while (*begin);
}

static size_t write_spell_modifier(spell * sp, int flag, const char * str, bool cont, char * bufp, size_t size) {
    if (sp->sptyp & flag) {
        size_t bytes = 0;
        if (cont) {
            bytes = strlcpy(bufp, ", ", size);
        }
        else {
            bytes = strlcpy(bufp, " ", size);
        }
        bytes += strlcpy(bufp + bytes, str, size - bytes);
        return bytes;
    }
    return 0;
}

static void nr_spell(FILE * F, spellbook_entry * sbe, const struct locale *lang)
{
    int bytes, k, itemanz, costtyp;
    char buf[4096];
    char *startp, *bufp = buf;
    size_t size = sizeof(buf) - 1;
    spell * sp = sbe->sp;
    const char *params = sp->parameter;

    rnl(F);
    centre(F, spell_name(sp, lang), true);
    rnl(F);
    rparagraph(F, LOC(lang, "nr_spell_description"), 0, 0, 0);
    rparagraph(F, spell_info(sp, lang), 2, 0, 0);

    bytes = (int)strlcpy(bufp, LOC(lang, "nr_spell_type"), size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    if (size) {
        *bufp++ = ' ';
        --size;
    }
    if (sp->sptyp & PRECOMBATSPELL) {
        bytes = (int)strlcpy(bufp, LOC(lang, "sptype_precombat"), size);
    }
    else if (sp->sptyp & COMBATSPELL) {
        bytes = (int)strlcpy(bufp, LOC(lang, "sptype_combat"), size);
    }
    else if (sp->sptyp & POSTCOMBATSPELL) {
        bytes = (int)strlcpy(bufp, LOC(lang, "sptype_postcombat"), size);
    }
    else {
        bytes = (int)strlcpy(bufp, LOC(lang, "sptype_normal"), size);
    }
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();
    *bufp = 0;
    rparagraph(F, buf, 0, 0, 0);

    sprintf(buf, "%s %d", LOC(lang, "nr_spell_level"), sbe->level);
    rparagraph(F, buf, 0, 0, 0);

    sprintf(buf, "%s %d", LOC(lang, "nr_spell_rank"), sp->rank);
    rparagraph(F, buf, 0, 0, 0);

    rparagraph(F, LOC(lang, "nr_spell_components"), 0, 0, 0);
    for (k = 0; sp->components[k].type; ++k) {
        const resource_type *rtype = sp->components[k].type;
        itemanz = sp->components[k].amount;
        costtyp = sp->components[k].cost;
        if (itemanz > 0) {
            size = sizeof(buf) - 1;
            bufp = buf;
            if (sp->sptyp & SPELLLEVEL) {
                bytes =
                    _snprintf(bufp, size, "  %d %s", itemanz, LOC(lang, resourcename(rtype,
                    itemanz != 1)));
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
                if (costtyp == SPC_LEVEL || costtyp == SPC_LINEAR) {
                    bytes = _snprintf(bufp, size, " * %s", LOC(lang, "nr_level"));
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                }
            }
            else {
                bytes = _snprintf(bufp, size, "%d %s", itemanz, LOC(lang, resourcename(rtype, itemanz != 1)));
                if (wrptr(&bufp, &size, bytes) != 0) {
                    WARN_STATIC_BUFFER();
                }
            }
            *bufp = 0;
            rparagraph(F, buf, 2, 2, '-');
        }
    }

    size = sizeof(buf) - 1;
    bufp = buf;
    bytes = (int)strlcpy(buf, LOC(lang, "nr_spell_modifiers"), size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    startp = bufp;
    bytes = (int)write_spell_modifier(sp, FARCASTING, LOC(lang, "smod_far"), startp != bufp, bufp, size);
    if (bytes && wrptr(&bufp, &size, bytes) != 0) {
        WARN_STATIC_BUFFER();
    }
    bytes = (int)write_spell_modifier(sp, OCEANCASTABLE, LOC(lang, "smod_sea"), startp != bufp, bufp, size);
    if (bytes && wrptr(&bufp, &size, bytes) != 0) {
        WARN_STATIC_BUFFER();
    }
    bytes = (int)write_spell_modifier(sp, ONSHIPCAST, LOC(lang, "smod_ship"), startp != bufp, bufp, size);
    if (bytes && wrptr(&bufp, &size, bytes) != 0) {
        WARN_STATIC_BUFFER();
    }
    bytes = (int)write_spell_modifier(sp, NOTFAMILIARCAST, LOC(lang, "smod_nofamiliar"), startp != bufp, bufp, size);
    if (bytes && wrptr(&bufp, &size, bytes) != 0) {
        WARN_STATIC_BUFFER();
    }
    if (startp == bufp) {
        bytes = (int)write_spell_modifier(sp, NOTFAMILIARCAST, LOC(lang, "smod_none"), startp != bufp, bufp, size);
        if (bytes && wrptr(&bufp, &size, bytes) != 0) {
            WARN_STATIC_BUFFER();
        }
    }
    *bufp = 0;
    rparagraph(F, buf, 0, 0, 0);

    rparagraph(F, LOC(lang, "nr_spell_syntax"), 0, 0, 0);

    bufp = buf;
    size = sizeof(buf) - 1;

    if (sp->sptyp & ISCOMBATSPELL) {
        bytes = (int)strlcpy(bufp, LOC(lang, keyword(K_COMBATSPELL)), size);
    }
    else {
        bytes = (int)strlcpy(bufp, LOC(lang, keyword(K_CAST)), size);
    }
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    /* Reihenfolge beachten: Erst REGION, dann STUFE! */
    if (sp->sptyp & FARCASTING) {
        bytes = _snprintf(bufp, size, " [%s x y]", LOC(lang, parameters[P_REGION]));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    if (sp->sptyp & SPELLLEVEL) {
        bytes = _snprintf(bufp, size, " [%s n]", LOC(lang, parameters[P_LEVEL]));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }

    bytes = (int)_snprintf(bufp, size, " \"%s\"", spell_name(sp, lang));
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

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
        int i, maxparam = 0;
        const char *locp;
        const char *syntaxp = sp->syntax;

        if (cp == 'u') {
            targetp = targets + 1;
            locp = LOC(lang, targetp->vars);
            bytes = (int)_snprintf(bufp, size, " <%s>", locp);
            if (*params == '+') {
                ++params;
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
                bytes = (int)_snprintf(bufp, size, " [<%s> ...]", locp);
            }
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
        else if (cp == 's') {
            targetp = targets + 2;
            locp = LOC(lang, targetp->vars);
            bytes = (int)_snprintf(bufp, size, " <%s>", locp);
            if (*params == '+') {
                ++params;
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
                bytes = (int)_snprintf(bufp, size, " [<%s> ...]", locp);
            }
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
        else if (cp == 'r') {
            bytes = (int)strlcpy(bufp, " <x> <y>", size);
            if (*params == '+') {
                ++params;
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
                bytes = (int)strlcpy(bufp, " [<x> <y> ...]", size);
            }
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
        else if (cp == 'b') {
            targetp = targets + 3;
            locp = LOC(lang, targetp->vars);
            bytes = (int)_snprintf(bufp, size, " <%s>", locp);
            if (*params == '+') {
                ++params;
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
                bytes = (int)_snprintf(bufp, size, " [<%s> ...]", locp);
            }
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
        else if (cp == 'k') {
            if (*params == 'c') {
                /* skip over a potential id */
                ++params;
            }
            for (targetp = targets; targetp->flag; ++targetp) {
                if (sp->sptyp & targetp->flag)
                    ++maxparam;
            }
            if (maxparam > 1) {
                bytes = (int)strlcpy(bufp, " (", size);
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
            }
            i = 0;
            for (targetp = targets; targetp->flag; ++targetp) {
                if (sp->sptyp & targetp->flag) {
                    if (i++ != 0) {
                        bytes = (int)strlcpy(bufp, " |", size);
                        if (wrptr(&bufp, &size, bytes) != 0)
                            WARN_STATIC_BUFFER();
                    }
                    if (targetp->param) {
                        locp = LOC(lang, targetp->vars);
                        bytes =
                            (int)_snprintf(bufp, size, " %s <%s>", parameters[targetp->param],
                            locp);
                        if (*params == '+') {
                            ++params;
                            if (wrptr(&bufp, &size, bytes) != 0)
                                WARN_STATIC_BUFFER();
                            bytes = (int)_snprintf(bufp, size, " [<%s> ...]", locp);
                        }
                    }
                    else {
                        bytes =
                            (int)_snprintf(bufp, size, " %s", parameters[targetp->param]);
                    }
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                }
            }
            if (maxparam > 1) {
                bytes = (int)strlcpy(bufp, " )", size);
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
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
                strncpy(substr, syntaxp, cstr - syntaxp);
                substr[cstr - syntaxp] = 0;
                locp = LOC(lang, mkname("spellpar", substr));
                syntaxp = substr + 1;
            }
            bytes = (int)_snprintf(bufp, size, " <%s>", locp);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }
    *bufp = 0;
    rparagraph(F, buf, 2, 0, 0);
    rnl(F);
}

void sparagraph(strlist ** SP, const char *s, int indent, char mark)
{

    /* Die Liste SP wird mit dem String s aufgefuellt, mit indent und einer
     * mark, falls angegeben. SP wurde also auf 0 gesetzt vor dem Aufruf.
     * Vgl. spunit (). */

    int i, j, width;
    int firstline;
    static char buf[REPORTWIDTH + 1]; // FIXME: static return value

    width = REPORTWIDTH - indent;
    firstline = 1;

    for (;;) {
        i = 0;

        do {
            j = i;
            while (s[j] && s[j] != ' ')
                j++;
            if (j > width) {

                /* j zeigt auf das ende der aktuellen zeile, i zeigt auf den anfang der
                 * n�chsten zeile. existiert ein wort am anfang der zeile, welches
                 * l�nger als eine zeile ist, muss dieses hier abgetrennt werden. */

                if (i == 0)
                    i = width - 1;
                break;
            }
            i = j + 1;
        } while (s[j]);

        for (j = 0; j != indent; j++)
            buf[j] = ' ';

        if (firstline && mark)
            buf[indent - 2] = mark;

        for (j = 0; j != i - 1; j++)
            buf[indent + j] = s[j];
        buf[indent + j] = 0;

        addstrlist(SP, buf);

        if (s[i - 1] == 0)
            break;

        s += i;
        firstline = 0;
    }
}

static void
nr_curses(FILE * F, const faction * viewer, const void *obj, objtype_t typ,
int indent)
{
    attrib *a = NULL;
    int self = 0;
    region *r;

    /* Die Sichtbarkeit eines Zaubers und die Zaubermeldung sind bei
     * Geb�uden und Schiffen je nach, ob man Besitzer ist, verschieden.
     * Bei Einheiten sieht man Wirkungen auf eigene Einheiten immer.
     * Spezialf�lle (besonderes Talent, verursachender Magier usw. werde
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
        /* fehler */
    }

    for (; a; a = a->next) {
        char buf[4096];
        message *msg;

        if (fval(a->type, ATF_CURSE)) {
            curse *c = (curse *)a->data.v;

            if (c->type->cansee) {
                self = c->type->cansee(viewer, obj, typ, c, self);
            }
            msg = msg_curse(c, obj, typ, self);

            if (msg) {
                rnl(F);
                nr_render(msg, viewer->locale, buf, sizeof(buf), viewer);
                rparagraph(F, buf, indent, 2, 0);
                msg_release(msg);
            }
        }
        else if (a->type == &at_effect && self) {
            effect_data *data = (effect_data *)a->data.v;
            if (data->value > 0) {
                msg = msg_message("nr_potion_effect", "potion left",
                    data->type->itype->rtype, data->value);
                nr_render(msg, viewer->locale, buf, sizeof(buf), viewer);
                rparagraph(F, buf, indent, 2, 0);
                msg_release(msg);
            }
        }
    }
}

static void rps_nowrap(FILE * F, const char *s)
{
    const char *x = s;
    size_t indent = 0;

    if (!x) return;

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
        rpc(F, *s++, 1);
    }
}

static void
nr_unit(FILE * F, const faction * f, const unit * u, int indent, int mode)
{
    attrib *a_otherfaction;
    char marker;
    int dh;
    bool isbattle = (bool)(mode == see_battle);
    char buf[8192];

    if (fval(u_race(u), RCF_INVISIBLE))
        return;

    {
        rnl(F);
        dh = bufunit(f, u, indent, mode, buf, sizeof(buf));
    }

    a_otherfaction = a_find(u->attribs, &at_otherfaction);

    if (u->faction == f) {
        marker = '*';
    }
    else if (is_allied(u->faction, f)) {
        marker = 'o';
    }
    else if (a_otherfaction && f != u->faction
        && get_otherfaction(a_otherfaction) == f && !fval(u, UFL_ANON_FACTION)) {
        marker = '!';
    }
    else {
        if (dh && !fval(u, UFL_ANON_FACTION)) {
            marker = '+';
        }
        else {
            marker = '-';
        }
    }
    rparagraph(F, buf, indent, 0, marker);

    if (!isbattle) {
        nr_curses(F, f, u, TYP_UNIT, indent);
    }
}

static void
rp_messages(FILE * F, message_list * msgs, faction * viewer, int indent,
bool categorized)
{
    nrsection *section;
    if (!msgs)
        return;
    for (section = sections; section; section = section->next) {
        int k = 0;
        struct mlist *m = msgs->begin;
        while (m) {
            /* messagetype * mt = m->type; */
            if (!categorized || strcmp(nr_section(m->msg), section->name) == 0) {
                char lbuf[8192];

                if (!k && categorized) {
                    const char *section_title;
                    char cat_identifier[24];

                    rnl(F);
                    sprintf(cat_identifier, "section_%s", section->name);
                    section_title = LOC(viewer->locale, cat_identifier);
                    centre(F, section_title, true);
                    rnl(F);
                    k = 1;
                }
                nr_render(m->msg, viewer->locale, lbuf, sizeof(lbuf), viewer);
                rparagraph(F, lbuf, indent, 2, 0);
            }
            m = m->next;
        }
        if (!categorized)
            break;
    }
}

static void rp_battles(FILE * F, faction * f)
{
    if (f->battles != NULL) {
        struct bmsg *bm = f->battles;
        rnl(F);
        centre(F, LOC(f->locale, "section_battle"), false);
        rnl(F);

        while (bm) {
            char buf[256];
            RENDER(f, buf, sizeof(buf), ("battle::header", "region", bm->r));
            rnl(F);
            centre(F, buf, true);
            rnl(F);
            rp_messages(F, bm->msgs, f, 0, false);
            bm = bm->next;
        }
    }
}

static void prices(FILE * F, const region * r, const faction * f)
{
    const luxury_type *sale = NULL;
    struct demand *dmd;
    message *m;
    int bytes, n = 0;
    char buf[4096], *bufp = buf;
    size_t size = sizeof(buf) - 1;

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

    bytes = (int)nr_render(m, f->locale, bufp, size, f);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();
    msg_release(m);

    if (n > 0) {
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_trade_intro"), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();

        for (dmd = r->land->demands; dmd; dmd = dmd->next){
            if (dmd->value > 0) {
                m = msg_message("nr_market_price", "product price",
                    dmd->type->itype->rtype, dmd->value * dmd->type->price);
                bytes = (int)nr_render(m, f->locale, bufp, size, f);
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
                msg_release(m);
                n--;
                if (n == 0) {
                    bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_trade_end"),
                        size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                } else if (n == 1) {
                    bytes = (int)strlcpy(bufp, " ", size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_trade_final"),
                        size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    bytes = (int)strlcpy(bufp, " ", size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                } else {
                    bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_trade_next"),
                        size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    bytes = (int)strlcpy(bufp, " ", size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                }
            }
        }
    }
    /* Schreibe Paragraphen */
    *bufp = 0;
    rparagraph(F, buf, 0, 0, 0);

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

static void describe(FILE * F, const seen_region * sr, faction * f)
{
    const region *r = sr->r;
    int n;
    bool dh;
    direction_t d;
    int trees;
    int saplings;
    attrib *a;
    const char *tname;
    struct edge {
        struct edge *next;
        char *name;
        bool transparent;
        bool block;
        bool exist[MAXDIRECTIONS];
        direction_t lastd;
    } *edges = NULL, *e;
    bool see[MAXDIRECTIONS];
    char buf[8192];
    char *bufp = buf;
    size_t size = sizeof(buf);
    int bytes;

    for (d = 0; d != MAXDIRECTIONS; d++) {
        /* Nachbarregionen, die gesehen werden, ermitteln */
        region *r2 = rconnect(r, d);
        connection *b;
        see[d] = true;
        if (!r2)
            continue;
        for (b = get_borders(r, r2); b;) {
            struct edge *e = edges;
            bool transparent = b->type->transparent(b, f);
            const char *name = b->type->name(b, r, f, GF_DETAILED | GF_ARTICLE);

            if (!transparent)
                see[d] = false;
            if (!see_border(b, f, r)) {
                b = b->next;
                continue;
            }
            while (e && (e->transparent != transparent || strcmp(name, e->name)))
                e = e->next;
            if (!e) {
                e = calloc(sizeof(struct edge), 1);
                e->name = _strdup(name);
                e->transparent = transparent;
                e->next = edges;
                edges = e;
            }
            e->lastd = d;
            e->exist[d] = true;
            b = b->next;
        }
    }

    bytes = (int)f_regionid(r, f, bufp, size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    if (sr->mode == see_travel) {
        bytes = _snprintf(bufp, size, " (%s)", LOC(f->locale, "see_travel"));
    }
    else if (sr->mode == see_neighbour) {
        bytes = _snprintf(bufp, size, " (%s)", LOC(f->locale, "see_neighbour"));
    }
    else if (sr->mode == see_lighthouse) {
        bytes = _snprintf(bufp, size, " (%s)", LOC(f->locale, "see_lighthouse"));
    }
    else {
        bytes = 0;
    }
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    /* Terrain */
    bytes = (int)strlcpy(bufp, ", ", size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    tname = terrain_name(r);
    bytes = (int)strlcpy(bufp, LOC(f->locale, tname), size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    /* Trees */
    trees = rtrees(r, 2);
    saplings = rtrees(r, 1);
    if (production(r)) {
        if (trees > 0 || saplings > 0) {
            bytes = _snprintf(bufp, size, ", %d/%d ", trees, saplings);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();

            if (fval(r, RF_MALLORN)) {
                if (trees == 1) {
                    bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_mallorntree"), size);
                }
                else {
                    bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_mallorntree_p"), size);
                }
            }
            else if (trees == 1) {
                bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_tree"), size);
            }
            else {
                bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_tree_p"), size);
            }
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }

    /* iron & stone */
    if (sr->mode == see_unit && f != (faction *)NULL) {
        resource_report result[MAX_RAWMATERIALS];
        int n, numresults = report_resources(sr, result, MAX_RAWMATERIALS, f);

        for (n = 0; n < numresults; ++n) {
            if (result[n].number >= 0 && result[n].level >= 0) {
                bytes = _snprintf(bufp, size, ", %d %s/%d", result[n].number,
                    LOC(f->locale, result[n].name), result[n].level);
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
            }
        }
    }

    /* peasants & silver */
    if (rpeasants(r)) {
        int n = rpeasants(r);
        bytes = _snprintf(bufp, size, ", %d", n);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();

        if (r->land->ownership) {
            const char *str =
                LOC(f->locale, mkname("morale", itoa10(r->land->morale)));
            bytes = _snprintf(bufp, size, " %s", str);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
        if (fval(r, RF_ORCIFIED)) {
            bytes = (int)strlcpy(bufp, " ", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();

            bytes =
                (int)strlcpy(bufp, LOC(f->locale, n == 1 ? "rc_orc" : "rc_orc_p"),
                size);
        }
        else {
            bytes = (int)strlcpy(bufp, " ", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            bytes =
                (int)strlcpy(bufp, LOC(f->locale, n == 1 ? "peasant" : "peasant_p"),
                size);
        }
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        if (is_mourning(r, turn + 1)) {
            bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_mourning"), size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }
    if (rmoney(r) && sr->mode >= see_travel) {
        bytes = _snprintf(bufp, size, ", %d ", rmoney(r));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes =
            (int)strlcpy(bufp, LOC(f->locale, resourcename(get_resourcetype(R_SILVER),
            rmoney(r) != 1)), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    /* Pferde */

    if (rhorses(r)) {
        bytes = _snprintf(bufp, size, ", %d ", rhorses(r));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes =
            (int)strlcpy(bufp, LOC(f->locale, resourcename(get_resourcetype(R_HORSE),
            (rhorses(r) > 1) ? GR_PLURAL : 0)), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    bytes = (int)strlcpy(bufp, ".", size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    if (r->display && r->display[0]) {
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, r->display, size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();

        n = r->display[strlen(r->display) - 1];
        if (n != '!' && n != '?' && n != '.') {
            bytes = (int)strlcpy(bufp, ".", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }

    if (rule_region_owners()) {
        const faction *owner = region_get_owner(r);
        message *msg;

        if (owner != NULL) {
            msg = msg_message("nr_region_owner", "faction", owner);
            bytes = (int)nr_render(msg, f->locale, bufp, size, f);
            msg_release(msg);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }
    a = a_find(r->attribs, &at_overrideroads);

    if (a) {
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, (char *)a->data.v, size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    else {
        int nrd = 0;

        /* Nachbarregionen, die gesehen werden, ermitteln */
        for (d = 0; d != MAXDIRECTIONS; d++) {
            if (see[d] && rconnect(r, d))
                nrd++;
        }
        /* list directions */

        dh = false;
        for (d = 0; d != MAXDIRECTIONS; d++) {
            if (see[d]) {
                region *r2 = rconnect(r, d);
                if (!r2)
                    continue;
                nrd--;
                if (dh) {
                    char regname[4096];
                    if (nrd == 0) {
                        bytes = (int)strlcpy(bufp, " ", size);
                        if (wrptr(&bufp, &size, bytes) != 0)
                            WARN_STATIC_BUFFER();
                        bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_nb_final"), size);
                    }
                    else {
                        bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_nb_next"), size);
                    }
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    bytes = (int)strlcpy(bufp, LOC(f->locale, directions[d]), size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    bytes = (int)strlcpy(bufp, " ", size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    f_regionid(r2, f, regname, sizeof(regname));
                    bytes = _snprintf(bufp, size, trailinto(r2, f->locale), regname);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                }
                else {
                    bytes = (int)strlcpy(bufp, " ", size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    MSG(("nr_vicinitystart", "dir region", d, r2), bufp, size, f->locale,
                        f);
                    bufp += strlen(bufp);
                    dh = true;
                }
            }
        }
        /* Spezielle Richtungen */
        for (a = a_find(r->attribs, &at_direction); a && a->type == &at_direction;
            a = a->next) {
            spec_direction *d = (spec_direction *)(a->data.v);
            bytes = (int)strlcpy(bufp, " ", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, LOC(f->locale, d->desc), size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, " (\"", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, LOC(f->locale, d->keyword), size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, "\")", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, ".", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            dh = 1;
        }
    }
    rnl(F);
    *bufp = 0;
    rparagraph(F, buf, 0, 0, 0);

    if (sr->mode == see_unit && is_astral(r) &&
        !is_cursed(r->attribs, C_ASTRALBLOCK, 0)) {
        /* Sonderbehandlung Teleport-Ebene */
        region_list *rl = astralregions(r, inhabitable);
        region_list *rl2;

        if (rl) {
            bufp = buf;
            size = sizeof(buf) - 1;

            // this localization might not work for every language but is fine for de and en
            bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_schemes_prefix"), size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            rl2 = rl;
            while (rl2) {
                bytes = (int)f_regionid(rl2->data, f, bufp, size);
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
                rl2 = rl2->next;
                if (rl2) {
                    bytes = (int)strlcpy(bufp, ", ", size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                }
            }
            bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_schemes_postfix"), size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            free_regionlist(rl);
            /* Schreibe Paragraphen */
            rnl(F);
            *bufp = 0;
            rparagraph(F, buf, 0, 0, 0);
        }
    }

    n = 0;

    /* Wirkungen permanenter Spr�che */
    nr_curses(F, f, r, TYP_REGION, 0);

    /* Produktionsreduktion */
    a = a_find(r->attribs, &at_reduceproduction);
    if (a) {
        const char *str = LOC(f->locale, "nr_reduced_production");
        rparagraph(F, str, 0, 0, 0);
    }

    if (edges)
        rnl(F);
    for (e = edges; e; e = e->next) {
        bool first = true;
        message *msg;

        bufp = buf;
        size = sizeof(buf) - 1;
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            if (!e->exist[d])
                continue;
            // this localization might not work for every language but is fine for de and en
            if (first)
                bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_borderlist_prefix"), size);
            else if (e->lastd == d)
                bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_borderlist_lastfix"), size);
            else
                bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_borderlist_infix"), size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, LOC(f->locale, directions[d]), size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            first = false;
        }
        // TODO name is localized? Works for roads anyway...
        msg = msg_message("nr_borderlist_postfix", "transparent object",
            e->transparent, e->name);
        bytes = (int)nr_render(msg, f->locale, bufp, size, f);
        msg_release(msg);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();

        *bufp = 0;
        rparagraph(F, buf, 0, 0, 0);
    }
    if (edges) {
        while (edges) {
            e = edges->next;
            free(edges->name);
            free(edges);
            edges = e;
        }
    }
}

static void statistics(FILE * F, const region * r, const faction * f)
{
    const unit *u;
    int number = 0, p = rpeasants(r);
    message *m;
    item *itm, *items = NULL;
    char buf[4096];

    /* count */
    for (u = r->units; u; u = u->next) {
        if (u->faction == f && !fval(u_race(u), RCF_INVISIBLE)) {
            for (itm = u->items; itm; itm = itm->next) {
                i_change(&items, itm->type, itm->number);
            }
            number += u->number;
        }
    }
    /* print */
    rnl(F);
    m = msg_message("nr_stat_header", "region", r);
    nr_render(m, f->locale, buf, sizeof(buf), f);
    msg_release(m);
    rparagraph(F, buf, 0, 0, 0);
    rnl(F);

    /* Region */
    if (skill_enabled(SK_ENTERTAINMENT) && fval(r->terrain, LAND_REGION)
        && rmoney(r)) {
        m = msg_message("nr_stat_maxentertainment", "max", entertainmoney(r));
        nr_render(m, f->locale, buf, sizeof(buf), f);
        rparagraph(F, buf, 2, 2, 0);
        msg_release(m);
    }
    if (production(r) && (!fval(r->terrain, SEA_REGION)
        || f->race == get_race(RC_AQUARIAN))) {
        if (markets_module()) {     /* hack */
            m =
                msg_message("nr_stat_salary_new", "max", wage(r, NULL, NULL, turn + 1));
        }
        else {
            m = msg_message("nr_stat_salary", "max", wage(r, f, f->race, turn + 1));
        }
        nr_render(m, f->locale, buf, sizeof(buf), f);
        rparagraph(F, buf, 2, 2, 0);
        msg_release(m);
    }

    if (p) {
        m = msg_message("nr_stat_recruits", "max", p / RECRUITFRACTION);
        nr_render(m, f->locale, buf, sizeof(buf), f);
        rparagraph(F, buf, 2, 2, 0);
        msg_release(m);

        if (!markets_module()) {
            if (buildingtype_exists(r, bt_find("caravan"), true)) {
                m = msg_message("nr_stat_luxuries", "max", (p * 2) / TRADE_FRACTION);
            }
            else {
                m = msg_message("nr_stat_luxuries", "max", p / TRADE_FRACTION);
            }
            nr_render(m, f->locale, buf, sizeof(buf), f);
            rparagraph(F, buf, 2, 2, 0);
            msg_release(m);
        }

        if (r->land->ownership) {
            m = msg_message("nr_stat_morale", "morale", r->land->morale);
            nr_render(m, f->locale, buf, sizeof(buf), f);
            rparagraph(F, buf, 2, 2, 0);
            msg_release(m);
        }

    }
    /* info about units */

    m = msg_message("nr_stat_people", "max", number);
    nr_render(m, f->locale, buf, sizeof(buf), f);
    rparagraph(F, buf, 2, 2, 0);
    msg_release(m);

    for (itm = items; itm; itm = itm->next) {
        sprintf(buf, "%s: %d",
            LOC(f->locale, resourcename(itm->type->rtype, GR_PLURAL)), itm->number);
        rparagraph(F, buf, 2, 2, 0);
    }
    while (items)
        i_free(i_remove(&items, items));
}

static void durchreisende(FILE * F, const region * r, const faction * f)
{
    if (fval(r, RF_TRAVELUNIT)) {
        attrib *abegin = a_find(r->attribs, &at_travelunit), *a;
        int counter = 0, maxtravel = 0;
        char buf[8192];
        char *bufp = buf;
        int bytes;
        size_t size = sizeof(buf) - 1;

        /* How many are we listing? For grammar. */
        for (a = abegin; a && a->type == &at_travelunit; a = a->next) {
            unit *u = (unit *)a->data.v;

            if (r != u->region && (!u->ship || ship_owner(u->ship) == u)) {
                if (cansee_durchgezogen(f, r, u, 0)) {
                    ++maxtravel;
                }
            }
        }

        if (maxtravel == 0) {
            return;
        }

        /* Auflisten. */
        rnl(F);

        for (a = abegin; a && a->type == &at_travelunit; a = a->next) {
            unit *u = (unit *)a->data.v;

            if (r != u->region && (!u->ship || ship_owner(u->ship) == u)) {
                if (cansee_durchgezogen(f, r, u, 0)) {
                    ++counter;
                    if (u->ship != NULL) {
#ifdef GERMAN_FLUFF_ENABLED
                        if (strcmp("de", f->locale->name)==0) {
                            if (counter == 1) {
                                bytes = (int)strlcpy(bufp, "Die ", size);
                            }
                            else {
                                bytes = (int)strlcpy(bufp, "die ", size);
                            }
                            if (wrptr(&bufp, &size, bytes) != 0) {
                                WARN_STATIC_BUFFER();
                                break;
                            }
                        }
#endif
                        bytes = (int)strlcpy(bufp, shipname(u->ship), size);
                    }
                    else {
                        bytes = (int)strlcpy(bufp, unitname(u), size);
                    }
                    if (wrptr(&bufp, &size, bytes) != 0) {
                        WARN_STATIC_BUFFER();
                        break;
                    }

                    if (counter + 1 < maxtravel) {
                        bytes = (int)strlcpy(bufp, ", ", size);
                        if (wrptr(&bufp, &size, bytes) != 0) {
                            WARN_STATIC_BUFFER();
                            break;
                        }
                    }
                    else if (counter + 1 == maxtravel) {
                        bytes = (int)strlcpy(bufp, LOC(f->locale, "list_and"), size);
                        if (wrptr(&bufp, &size, bytes) != 0) {
                            WARN_STATIC_BUFFER();
                            break;
                        }
                    }
                }
            }
        }
        /* TODO: finish localization */
        if (size > 0) {
            if (maxtravel == 1) {
                bytes = _snprintf(bufp, size, " %s", LOC(f->locale, "has_moved_one"));
            }
            else {
                bytes = _snprintf(bufp, size, " %s", LOC(f->locale, "has_moved_many"));
            }
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
        *bufp = 0;
        rparagraph(F, buf, 0, 0, 0);
    }
}

static int buildingmaintenance(const building * b, const resource_type * rtype)
{
    const building_type *bt = b->type;
    int c, cost = 0;
    static bool init = false;
    static const curse_type *nocost_ct;
    if (!init) {
        init = true;
        nocost_ct = ct_find("nocostbuilding");
    }
    if (curse_active(get_curse(b->attribs, nocost_ct))) {
        return 0;
    }
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
report_template(const char *filename, report_context * ctx, const char *charset)
{
    const resource_type *rsilver = get_resourcetype(R_SILVER);
    faction *f = ctx->f;
    region *r;
    FILE *F = fopen(filename, "wt");
    seen_region *sr = NULL;
    char buf[8192], *bufp;
    size_t size;
    int bytes;
    bool utf8 = _strcmpl(charset, "utf8") == 0 || _strcmpl(charset, "utf-8") == 0;

    if (F == NULL) {
        perror(filename);
        return -1;
    }

    if (utf8) {
        const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf, 0 };
        fwrite(utf8_bom, 1, 3, F);
    }

    rps_nowrap(F, "");
    rnl(F);
    rps_nowrap(F, LOC(f->locale, "nr_template"));
    rnl(F);
    rps_nowrap(F, "");
    rnl(F);

    sprintf(buf, "%s %s \"%s\"", LOC(f->locale, "ERESSEA"), factionid(f),
        LOC(f->locale, "enterpasswd"));
    rps_nowrap(F, buf);
    rnl(F);

    rps_nowrap(F, "");
    rnl(F);
    sprintf(buf, "; ECHECK -l -w4 -r%d -v%s", f->race->recruitcost,
        ECHECK_VERSION);
    /* -v3.4: ECheck Version 3.4.x */
    rps_nowrap(F, buf);
    rnl(F);

    for (r = ctx->first; sr == NULL && r != ctx->last; r = r->next) {
        sr = find_seen(ctx->seen, r);
    }

    for (; sr != NULL; sr = sr->next) {
        region *r = sr->r;
        unit *u;
        int dh = 0;

        if (sr->mode < see_unit)
            continue;

        for (u = r->units; u; u = u->next) {
            if (u->faction == f && !fval(u_race(u), RCF_INVISIBLE)) {
                order *ord;
                if (!dh) {
                    plane *pl = getplane(r);
                    int nx = r->x, ny = r->y;

                    pnormalize(&nx, &ny, pl);
                    adjust_coordinates(f, &nx, &ny, pl, r);
                    rps_nowrap(F, "");
                    rnl(F);
                    if (pl && pl->id != 0) {
                        sprintf(buf, "%s %d,%d,%d ; %s", LOC(f->locale,
                            parameters[P_REGION]), nx, ny, pl->id, rname(r, f->locale));
                    }
                    else {
                        sprintf(buf, "%s %d,%d ; %s", LOC(f->locale, parameters[P_REGION]),
                            nx, ny, rname(r, f->locale));
                    }
                    rps_nowrap(F, buf);
                    rnl(F);
                    sprintf(buf, "; ECheck Lohn %d", wage(r, f, f->race, turn + 1));
                    rps_nowrap(F, buf);
                    rnl(F);
                    rps_nowrap(F, "");
                    rnl(F);
                }
                dh = 1;

                bufp = buf;
                size = sizeof(buf) - 1;
                bytes = _snprintf(bufp, size, "%s %s;    %s [%d,%d$",
                    LOC(u->faction->locale, parameters[P_UNIT]),
                    unitid(u), unit_getname(u), u->number, get_money(u));
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
                if (u->building && building_owner(u->building) == u) {
                    building *b = u->building;
                    int cost = buildingmaintenance(b, rsilver);

                    if (cost > 0) {
                        bytes = (int)strlcpy(bufp, ",U", size);
                        if (wrptr(&bufp, &size, bytes) != 0)
                            WARN_STATIC_BUFFER();
                        bytes = (int)strlcpy(bufp, itoa10(cost), size);
                        if (wrptr(&bufp, &size, bytes) != 0)
                            WARN_STATIC_BUFFER();
                    }
                }
                else if (u->ship) {
                    if (ship_owner(u->ship) == u) {
                        bytes = (int)strlcpy(bufp, ",S", size);
                    }
                    else {
                        bytes = (int)strlcpy(bufp, ",s", size);
                    }
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    bytes = (int)strlcpy(bufp, shipid(u->ship), size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                }
                if (lifestyle(u) == 0) {
                    bytes = (int)strlcpy(bufp, ",I", size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                }
                bytes = (int)strlcpy(bufp, "]", size);
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();

                *bufp = 0;
                rps_nowrap(F, buf);
                rnl(F);

                for (ord = u->old_orders; ord; ord = ord->next) {
                    /* this new order will replace the old defaults */
                    strcpy(buf, "   ");
                    write_order(ord, buf + 2, sizeof(buf) - 2);
                    rps_nowrap(F, buf);
                    rnl(F);
                }
                for (ord = u->orders; ord; ord = ord->next) {
                    if (u->old_orders && is_repeated(ord))
                        continue;           /* unit has defaults */
                    if (is_persistent(ord)) {
                        strcpy(buf, "   ");
                        write_order(ord, buf + 2, sizeof(buf) - 2);
                        rps_nowrap(F, buf);
                        rnl(F);
                    }
                }

                /* If the lastorder begins with an @ it should have
                 * been printed in the loop before. */
            }
        }
    }
    rps_nowrap(F, "");
    rnl(F);
    strcpy(buf, LOC(f->locale, parameters[P_NEXT]));
    rps_nowrap(F, buf);
    rnl(F);
    fclose(F);
    return 0;
}

static void
show_allies(const faction * f, const ally * allies, char *buf, size_t size)
{
    int allierte = 0;
    int i = 0, h, hh = 0;
    int bytes, dh = 0;
    const ally *sf;
    char *bufp = buf;             /* buf already contains data */

    --size;                       /* leave room for a null-terminator */

    for (sf = allies; sf; sf = sf->next) {
        int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
        if (mode > 0)
            ++allierte;
    }

    for (sf = allies; sf; sf = sf->next) {
        int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
        if (mode <= 0)
            continue;
        i++;
        if (dh) {
            if (i == allierte) {
                bytes = (int)strlcpy(bufp, LOC(f->locale, "list_and"), size);
            }
            else {
                bytes = (int)strlcpy(bufp, ", ", size);
            }
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
        dh = 1;
        hh = 0;
        bytes = (int)strlcpy(bufp, factionname(sf->faction), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, " (", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        if ((mode & HELP_ALL) == HELP_ALL) {
            bytes = (int)strlcpy(bufp, LOC(f->locale, parameters[P_ANY]), size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
        else {
            for (h = 1; h < HELP_ALL; h *= 2) {
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
                        bytes = (int)strlcpy(bufp, ", ", size);
                        if (wrptr(&bufp, &size, bytes) != 0)
                            WARN_STATIC_BUFFER();
                    }
                    bytes = (int)strlcpy(bufp, LOC(f->locale, parameters[p]), size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    hh = 1;
                }
            }
        }
        bytes = (int)strlcpy(bufp, ")", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    bytes = (int)strlcpy(bufp, ".", size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();
    *bufp = 0;
}

static void allies(FILE * F, const faction * f)
{
    const group *g = f->groups;
    char buf[16384];

    if (f->allies) {
        int bytes;
        size_t size = sizeof(buf);
        bytes = _snprintf(buf, size, "%s ", LOC(f->locale, "faction_help"));
        size -= bytes;
        show_allies(f, f->allies, buf + bytes, size);
        rparagraph(F, buf, 0, 0, 0);
        rnl(F);
    }

    while (g) {
        if (g->allies) {
            int bytes;
            size_t size = sizeof(buf);
            bytes = _snprintf(buf, size, "%s %s ", g->name, LOC(f->locale, "group_help"));
            size -= bytes;
            show_allies(f, g->allies, buf + bytes, size);
            rparagraph(F, buf, 0, 0, 0);
            rnl(F);
        }
        g = g->next;
    }
}

static void guards(FILE * F, const region * r, const faction * see)
{
    /* die Partei  see  sieht dies; wegen
     * "unbekannte Partei", wenn man es selbst ist... */

    faction *guardians[512];

    int nextguard = 0;

    unit *u;
    int i;

    bool tarned = false;
    /* Bewachung */

    for (u = r->units; u; u = u->next) {
        if (is_guard(u, GUARD_ALL) != 0) {
            faction *f = u->faction;
            faction *fv = visible_faction(see, u);

            if (fv != f && see != fv) {
                f = fv;
            }

            if (f != see && fval(u, UFL_ANON_FACTION)) {
                tarned = true;
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

    if (nextguard || tarned) {
        char buf[8192];
        char *bufp = buf;
        size_t size = sizeof(buf) - 1;
        int bytes;

        bytes = (int)strlcpy(bufp, LOC(see->locale, "nr_guarding_prefix"), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();

        for (i = 0; i != nextguard + (tarned ? 1 : 0); ++i) {
            if (i != 0) {
                if (i == nextguard - (tarned ? 0 : 1)) {
                    bytes = (int)strlcpy(bufp, LOC(see->locale, "list_and"), size);
                }
                else {
                    bytes = (int)strlcpy(bufp, ", ", size);
                }
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
            }
            if (i < nextguard) {
                bytes = (int)strlcpy(bufp, factionname(guardians[i]), size);
            }
            else {
                bytes = (int)strlcpy(bufp, LOC(see->locale, "nr_guarding_unknown"), size);
            }
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
        bytes = (int)strlcpy(bufp, LOC(see->locale, "nr_guarding_postfix"), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        rnl(F);
        *bufp = 0;
        rparagraph(F, buf, 0, 0, 0);
    }
}

static void rpline(FILE * F)
{
    static char line[REPORTWIDTH + 1];
    if (line[0] != '-') {
        memset(line, '-', sizeof(line));
        line[REPORTWIDTH] = '\n';
    }
    fwrite(line, sizeof(char), sizeof(line), F);
}

static void list_address(FILE * F, const faction * uf, quicklist * seenfactions)
{
    int qi = 0;
    quicklist *flist = seenfactions;

    centre(F, LOC(uf->locale, "nr_addresses"), false);
    rnl(F);

    while (flist != NULL) {
        const faction *f = (const faction *)ql_get(flist, qi);
        if (!is_monsters(f)) {
            char buf[8192];
            char label = '-';

            sprintf(buf, "%s: %s; %s", factionname(f), f->email,
                f->banner ? f->banner : "");
            if (uf == f)
                label = '*';
            else if (is_allied(uf, f))
                label = 'o';
            else if (alliedfaction(NULL, uf, f, HELP_ALL))
                label = '+';
            rparagraph(F, buf, 4, 0, label);

        }
        ql_advance(&flist, &qi, 1);
    }
    rnl(F);
    rpline(F);
}

static void
nr_ship(FILE * F, const seen_region * sr, const ship * sh, const faction * f,
const unit * captain)
{
    const region *r = sr->r;
    char buffer[8192], *bufp = buffer;
    size_t size = sizeof(buffer) - 1;
    int bytes;
    char ch;

    rnl(F);

    if (captain && captain->faction == f) {
        int n = 0, p = 0;
        getshipweight(sh, &n, &p);
        n = (n + 99) / 100;         /* 1 Silber = 1 GE */

        bytes = _snprintf(bufp, size, "%s, %s, (%d/%d)", shipname(sh),
            LOC(f->locale, sh->type->_name), n, shipcapacity(sh) / 100);
    }
    else {
        bytes =
            _snprintf(bufp, size, "%s, %s", shipname(sh), LOC(f->locale,
            sh->type->_name));
    }
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    assert(sh->type->construction->improvement == NULL);  /* sonst ist construction::size nicht ship_type::maxsize */
    if (sh->size != sh->type->construction->maxsize) {
        bytes = _snprintf(bufp, size, ", %s (%d/%d)",
            LOC(f->locale, "nr_undercons"), sh->size,
            sh->type->construction->maxsize);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    if (sh->damage) {
        int percent =
            (sh->damage * 100 + DAMAGE_SCALE - 1) / (sh->size * DAMAGE_SCALE);
        bytes =
            _snprintf(bufp, size, ", %d%% %s", percent, LOC(f->locale, "nr_damaged"));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    if (!fval(r->terrain, SEA_REGION)) {
        if (sh->coast != NODIRECTION) {
            bytes = (int)strlcpy(bufp, ", ", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, LOC(f->locale, coasts[sh->coast]), size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }
    ch = 0;
    if (sh->display && sh->display[0]) {
        bytes = (int)strlcpy(bufp, "; ", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, sh->display, size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        ch = sh->display[strlen(sh->display) - 1];
    }
    if (ch != '!' && ch != '?' && ch != '.') {
        bytes = (int)strlcpy(bufp, ".", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    *bufp = 0;
    rparagraph(F, buffer, 2, 0, 0);

    nr_curses(F, f, sh, TYP_SHIP, 4);
}

static void
nr_building(FILE * F, const seen_region * sr, const building * b,
const faction * f)
{
    int i, bytes;
    const char *name, *bname, *billusion = NULL;
    const struct locale *lang = NULL;
    char buffer[8192], *bufp = buffer;
    message *msg;
    size_t size = sizeof(buffer) - 1;

    rnl(F);

    if (f)
        lang = f->locale;

    bytes =
        _snprintf(bufp, size, "%s, %s %d, ", buildingname(b), LOC(f->locale,
        "nr_size"), b->size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    report_building(b, &bname, &billusion);
    name = LOC(lang, billusion ? billusion : bname);
    bytes = (int)strlcpy(bufp, name, size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();
    if (billusion) {
        unit *owner = building_owner(b);
        if (owner && owner->faction == f) {
            /* illusion. report real type */
            name = LOC(lang, bname);
            bytes = _snprintf(bufp, size, " (%s)", name);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }

    if (b->size < b->type->maxsize) {
        bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_building_inprogress"), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    if (b->damage) {
        int percent = (b->damage * 100) / b->size;
        bytes = _snprintf(bufp, size, ", %d%% %s", percent, LOC(f->locale, "nr_damaged"));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }

    if (b->besieged > 0 && sr->mode >= see_lighthouse) {
        msg = msg_message("nr_building_besieged", "soldiers diff", b->besieged,
            b->besieged - b->size * SIEGEFACTOR);
        bytes = (int)nr_render(msg, f->locale, bufp, size, f);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        msg_release(msg);
    }
    i = 0;
    if (b->display && b->display[0]) {
        bytes = (int)strlcpy(bufp, "; ", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, b->display, size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        i = b->display[strlen(b->display) - 1];
    }

    if (i != '!' && i != '?' && i != '.') {
        bytes = (int)strlcpy(bufp, ".", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    *bufp = 0;
    rparagraph(F, buffer, 2, 0, 0);

    if (sr->mode < see_lighthouse)
        return;

    nr_curses(F, f, b, TYP_BUILDING, 4);
}

static void nr_paragraph(FILE * F, message * m, faction * f)
{
    int bytes;
    char buf[4096], *bufp = buf;
    size_t size = sizeof(buf) - 1;

    bytes = (int)nr_render(m, f->locale, bufp, size, f);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();
    msg_release(m);

    rparagraph(F, buf, 0, 0, 0);
}

int
report_plaintext(const char *filename, report_context * ctx,
const char *charset)
{
    int flag = 0;
    char ch;
    int anyunits, no_units, no_people;
    const struct region *r;
    faction *f = ctx->f;
    unit *u;
    char pzTime[64];
    attrib *a;
    message *m;
    unsigned char op;
    int bytes, ix = want(O_STATISTICS);
    int wants_stats = (f->options & ix);
    FILE *F = fopen(filename, "wt");
    seen_region *sr = NULL;
    char buf[8192];
    char *bufp;
    bool utf8 = _strcmpl(charset, "utf8") == 0 || _strcmpl(charset, "utf-8") == 0;
    size_t size;

    /* static variables can cope with writing for different turns */
    static int thisseason = -1;
    static int nextseason = -1;
    static int gamecookie = -1;
    if (gamecookie != global.cookie) {
        gamedate date;
        get_gamedate(turn + 1, &date);
        thisseason = date.season;
        get_gamedate(turn + 2, &date);
        nextseason = date.season;
        gamecookie = global.cookie;
    }

    if (F == NULL) {
        perror(filename);
        return -1;
    }
    if (utf8) {
        const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf, 0 };
        fwrite(utf8_bom, 1, 3, F);
    }

    strftime(pzTime, 64, "%A, %d. %B %Y, %H:%M", localtime(&ctx->report_time));
    m = msg_message("nr_header_date", "game date", game_name(), pzTime);
    nr_render(m, f->locale, buf, sizeof(buf), f);
    msg_release(m);
    centre(F, buf, true);

    centre(F, gamedate_season(f->locale), true);
    rnl(F);
    sprintf(buf, "%s, %s/%s (%s)", factionname(f),
        LOC(f->locale, rc_name_s(f->race, NAME_PLURAL)),
        LOC(f->locale, mkname("school", magic_school[f->magiegebiet])), f->email);
    centre(F, buf, true);
    if (f_get_alliance(f)) {
        centre(F, alliancename(f->alliance), true);
    }

    if (f->age <= 2) {
        const char *s;
        if (f->age <= 1) {
            ADDMSG(&f->msgs, msg_message("changepasswd", "value", f->passw));
        }
        RENDER(f, buf, sizeof(buf), ("newbie_password", "password", f->passw));
        rnl(F);
        centre(F, buf, true);
        s = locale_getstring(f->locale, "newbie_info_1");
        if (s) {
            rnl(F);
            centre(F, s, true);
        }
        s = locale_getstring(f->locale, "newbie_info_2");
        if (s) {
            rnl(F);
            centre(F, s, true);
        }
        if ((f->options & want(O_COMPUTER)) == 0) {
            f->options |= want(O_COMPUTER);
            s = locale_getstring(f->locale, "newbie_info_3");
            if (s) {
                rnl(F);
                centre(F, s, true);
            }
        }
    }
    rnl(F);
#if SCORE_MODULE
    if (f->options & want(O_SCORE) && f->age > DISPLAYSCORE) {
        RENDER(f, buf, sizeof(buf), ("nr_score", "score average", f->score,
            average_score_of_age(f->age, f->age / 24 + 1)));
        centre(F, buf, true);
    }
#endif
#ifdef COUNT_AGAIN
    no_units = 0;
    no_people = 0;
    for (u = f->units; u; u = u->nextF) {
        if (playerrace(u_race(u))) {
            ++no_people;
            no_units += u->number;
            assert(f == u->faction);
        }
    }
    if (no_units != f->no_units) {
        f->no_units = no_units;
    }
    if (no_people != f->num_people) {
        f->num_people = no_people;
    }
#else
    no_units = count_units(f);
    no_people = count_all(f);
    if (f->flags & FFL_NPC) {
        no_people = f->num_total;
    }
    else {
        no_people = f->num_people;
    }
#endif
    m = msg_message("nr_population", "population units limit", no_people, no_units, rule_faction_limit());
    nr_render(m, f->locale, buf, sizeof(buf), f);
    msg_release(m);
    centre(F, buf, true);
    if (f->race == get_race(RC_HUMAN)) {
        int maxmig = count_maxmigrants(f);
        if (maxmig > 0) {
            m =
                msg_message("nr_migrants", "units maxunits", count_migrants(f), maxmig);
            nr_render(m, f->locale, buf, sizeof(buf), f);
            msg_release(m);
            centre(F, buf, true);
        }
    }
    if (f_get_alliance(f)) {
        m =
            msg_message("nr_alliance", "leader name id age",
            alliance_get_leader(f->alliance), f->alliance->name, f->alliance->id,
            turn - f->alliance_joindate);
        nr_render(m, f->locale, buf, sizeof(buf), f);
        msg_release(m);
        centre(F, buf, true);
    }
  {
      int maxh = maxheroes(f);
      if (maxh) {
          message *msg =
              msg_message("nr_heroes", "units maxunits", countheroes(f), maxh);
          nr_render(msg, f->locale, buf, sizeof(buf), f);
          msg_release(msg);
          centre(F, buf, true);
      }
  }

  if (f->items != NULL) {
      message *msg = msg_message("nr_claims", "items", f->items);
      nr_render(msg, f->locale, buf, sizeof(buf), f);
      msg_release(msg);
      rnl(F);
      centre(F, buf, true);
  }

  /* Insekten-Winter-Warnung */
  if (f->race == get_race(RC_INSECT)) {
      if (thisseason == 0) {
          centre(F, LOC(f->locale, "nr_insectwinter"), true);
          rnl(F);
      }
      else {
          if (nextseason == 0) {
              centre(F, LOC(f->locale, "nr_insectfall"), true);
              rnl(F);
          }
      }
  }

  bufp = buf;
  size = sizeof(buf) - 1;
  bytes = _snprintf(buf, size, "%s:", LOC(f->locale, "nr_options"));
  if (wrptr(&bufp, &size, bytes) != 0)
      WARN_STATIC_BUFFER();
  for (op = 0; op != MAXOPTIONS; op++) {
      if (f->options & want(op) && options[op]) {
          bytes = (int)strlcpy(bufp, " ", size);
          if (wrptr(&bufp, &size, bytes) != 0)
              WARN_STATIC_BUFFER();
          bytes = (int)strlcpy(bufp, LOC(f->locale, options[op]), size);
          if (wrptr(&bufp, &size, bytes) != 0)
              WARN_STATIC_BUFFER();

          flag++;
      }
  }
  if (flag > 0) {
      rnl(F);
      *bufp = 0;
      centre(F, buf, true);
  }

  rp_messages(F, f->msgs, f, 0, true);
  rp_battles(F, f);
  a = a_find(f->attribs, &at_reportspell);
  if (a) {
      rnl(F);
      centre(F, LOC(f->locale, "section_newspells"), true);
      while (a && a->type == &at_reportspell) {
          spellbook_entry *sbe = (spellbook_entry *)a->data.v;
          nr_spell(F, sbe, f->locale);
          a = a->next;
      }
  }

  ch = 0;
  for (a = a_find(f->attribs, &at_showitem); a && a->type == &at_showitem;
      a = a->next) {
      const potion_type *ptype =
          resource2potion(((const item_type *)a->data.v)->rtype);
      const char *description = NULL;
      if (ptype != NULL) {
          const char *pname = resourcename(ptype->itype->rtype, 0);

          if (ch == 0) {
              rnl(F);
              centre(F, LOC(f->locale, "section_newpotions"), true);
              ch = 1;
          }

          rnl(F);
          centre(F, LOC(f->locale, pname), true);
          _snprintf(buf, sizeof(buf), "%s %d", LOC(f->locale, "nr_level"),
              ptype->level);
          centre(F, buf, true);
          rnl(F);

          bufp = buf;
          size = sizeof(buf) - 1;
          bytes = _snprintf(bufp, size, "%s: ", LOC(f->locale, "nr_herbsrequired"));
          if (wrptr(&bufp, &size, bytes) != 0)
              WARN_STATIC_BUFFER();

          if (ptype->itype->construction) {
              requirement *m = ptype->itype->construction->materials;
              while (m->number) {
                  bytes =
                      (int)strlcpy(bufp, LOC(f->locale, resourcename(m->rtype, 0)), size);
                  if (wrptr(&bufp, &size, bytes) != 0)
                      WARN_STATIC_BUFFER();
                  ++m;
                  if (m->number)
                      bytes = (int)strlcpy(bufp, ", ", size);
                  if (wrptr(&bufp, &size, bytes) != 0)
                      WARN_STATIC_BUFFER();
              }
          }
          *bufp = 0;
          centre(F, buf, true);
          rnl(F);
          if (description == NULL) {
              const char *potiontext = mkname("potion", pname);
              description = LOC(f->locale, potiontext);
          }
          centre(F, description, true);
      }
  }
  rnl(F);
  centre(F, LOC(f->locale, "nr_alliances"), false);
  rnl(F);

  allies(F, f);

  rpline(F);

  anyunits = 0;

  for (r = ctx->first; sr == NULL && r != ctx->last; r = r->next) {
      sr = find_seen(ctx->seen, r);
  }
  for (; sr != NULL; sr = sr->next) {
      region *r = sr->r;
      int stealthmod = stealth_modifier(sr->mode);
      building *b = r->buildings;
      ship *sh = r->ships;

      if (sr->mode < see_lighthouse)
          continue;
      /* Beschreibung */

      if (sr->mode == see_unit) {
          anyunits = 1;
          describe(F, sr, f);
          if (markets_module() && r->land) {
              const item_type *lux = r_luxury(r);
              const item_type *herb = r->land->herbtype;
              message *m = 0;
              if (herb && lux) {
                  m = msg_message("nr_market_info_p", "p1 p2",
                      lux ? lux->rtype : 0, herb ? herb->rtype : 0);
              }
              else if (lux || herb) {
                  m = msg_message("nr_market_info_s", "p1",
                      lux ? lux->rtype : herb->rtype);
              }
              if (m) {
                  rnl(F);
                  nr_paragraph(F, m, f);
              }
              /*  */
          }
          else {
              if (!fval(r->terrain, SEA_REGION) && rpeasants(r) / TRADE_FRACTION > 0) {
                  rnl(F);
                  prices(F, r, f);
              }
          }
          guards(F, r, f);
          durchreisende(F, r, f);
      }
      else {
          if (sr->mode == see_far) {
              describe(F, sr, f);
              guards(F, r, f);
              durchreisende(F, r, f);
          }
          else if (sr->mode == see_lighthouse) {
              describe(F, sr, f);
              durchreisende(F, r, f);
          }
          else {
              describe(F, sr, f);
              durchreisende(F, r, f);
          }
      }
      /* Statistik */

      if (wants_stats && sr->mode == see_unit)
          statistics(F, r, f);

      /* Nachrichten an REGION in der Region */

      if (sr->mode == see_unit || sr->mode == see_travel) {
          // TODO: Bug 2073
          message_list *mlist = r_getmessages(r, f);
          rp_messages(F, r->msgs, f, 0, true);
          if (mlist)
              rp_messages(F, mlist, f, 0, true);
      }

      /* report all units. they are pre-sorted in an efficient manner */
      u = r->units;
      while (b) {
          while (b && (!u || u->building != b)) {
              nr_building(F, sr, b, f);
              b = b->next;
          }
          if (b) {
              nr_building(F, sr, b, f);
              while (u && u->building == b) {
                  nr_unit(F, f, u, 6, sr->mode);
                  u = u->next;
              }
              b = b->next;
          }
      }
      while (u && !u->ship) {
          if (stealthmod > INT_MIN) {
              if (u->faction == f || cansee(f, r, u, stealthmod)) {
                  nr_unit(F, f, u, 4, sr->mode);
              }
          }
          assert(!u->building);
          u = u->next;
      }
      while (sh) {
          while (sh && (!u || u->ship != sh)) {
              nr_ship(F, sr, sh, f, NULL);
              sh = sh->next;
          }
          if (sh) {
              nr_ship(F, sr, sh, f, u);
              while (u && u->ship == sh) {
                  nr_unit(F, f, u, 6, sr->mode);
                  u = u->next;
              }
              sh = sh->next;
          }
      }

      assert(!u);

      rnl(F);
      rpline(F);
  }
  if (!is_monsters(f)) {
      if (!anyunits) {
          rnl(F);
          rparagraph(F, LOC(f->locale, "nr_youaredead"), 0, 2, 0);
      }
      else {
          list_address(F, f, ctx->addresses);
      }
  }
  fclose(F);
  return 0;
}

void base36conversion(void)
{
    region *r;
    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            if (forbiddenid(u->no)) {
                uunhash(u);
                u->no = newunitid();
                uhash(u);
            }
        }
    }
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

unit *can_find(faction * f, faction * f2)
{
    int key = f->no % FMAXHASH;
    struct fsee *fs = fsee[key];
    struct see *ss;
    if (f == f2)
        return f->units;
    while (fs && fs->f != f)
        fs = fs->nexthash;
    if (!fs)
        return NULL;
    ss = fs->see;
    while (ss && ss->seen != f2)
        ss = ss->next;
    if (ss) {
        /* bei TARNE PARTEI yxz muss die Partei von unit proof nicht
         * wirklich Partei f2 sein! */
        /* assert(ss->proof->faction==f2); */
        return ss->proof;
    }
    return NULL;
}

static void add_find(faction * f, unit * u, faction * f2)
{
    /* faction f sees f2 through u */
    int key = f->no % FMAXHASH;
    struct fsee **fp = &fsee[key];
    struct fsee *fs;
    struct see **sp;
    struct see *ss;
    while (*fp && (*fp)->f != f)
        fp = &(*fp)->nexthash;
    if (!*fp) {
        fs = *fp = calloc(sizeof(struct fsee), 1);
        fs->f = f;
    }
    else
        fs = *fp;
    sp = &fs->see;
    while (*sp && (*sp)->seen != f2)
        sp = &(*sp)->next;
    if (!*sp) {
        ss = *sp = calloc(sizeof(struct see), 1);
        ss->proof = u;
        ss->seen = f2;
    }
    else
        ss = *sp;
    ss->proof = u;
}

static void update_find(void)
{
    region *r;
    static bool initial = true;

    if (initial)
        for (r = regions; r; r = r->next) {
            unit *u;
            for (u = r->units; u; u = u->next) {
                faction *lastf = u->faction;
                unit *u2;
                for (u2 = r->units; u2; u2 = u2->next) {
                    if (u2->faction == lastf || u2->faction == u->faction)
                        continue;
                    if (seefaction(u->faction, r, u2, 0)) {
                        faction *fv = visible_faction(u->faction, u2);
                        lastf = fv;
                        add_find(u->faction, u2, fv);
                    }
                }
            }
        }
    initial = false;
}

bool kann_finden(faction * f1, faction * f2)
{
    update_find();
    return (bool)(can_find(f1, f2) != NULL);
}

/******* end summary ******/

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
