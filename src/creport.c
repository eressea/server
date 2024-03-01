#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <kernel/config.h>
#include <kernel/version.h>
#include "creport.h"

/* tweakable features */
#define RESOURCECOMPAT

#define BUFFERSIZE 32768
/* FIXME: riesig, wegen spionage-messages :-( */
static char g_bigbuf[BUFFERSIZE];

/* modules include */
#include <modules/score.h>

/* attributes include */
#include <attributes/follow.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/raceprefix.h>
#include <attributes/seenspell.h>
#include <attributes/stealth.h>

/* gamecode includes */
#include "alchemy.h"
#include "economy.h"
#include "guard.h"
#include "laws.h"
#include "market.h"
#include "move.h"
#include "recruit.h"
#include "reports.h"
#include "teleport.h"
#include "travelthru.h"

/* kernel includes */
#include "kernel/alliance.h"
#include "kernel/ally.h"
#include "kernel/calendar.h"
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
#include <kernel/attrib.h>
#include <util/base36.h>
#include <util/crmessage.h>
#include <util/language.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/nrmessage.h>

#include <filestream.h>
#include <selist.h>
#include <stream.h>
#include <strings.h>

#include <stb_ds.h>

/* libc includes */
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* imports */
bool opt_cr_absolute_coords = false;

/* globals */
#define C_REPORT_VERSION 69

struct locale *crtag_locale(void) {
    static struct locale * lang;
    static int config;
    if (config_changed(&config)) {
        const char *lname = config_get("creport.tags");
        lang = lname ? get_locale(lname) : default_locale;
    }
    return lang;
}

static const char *crtag(const char *key)
{
    const char *result;
    result = locale_string(crtag_locale(), key, false);
    return result ? result : key;
}
/*
 * translation table
 */
typedef struct translation {
    struct translation *next;
    char *key;
    const char *value;
} translation;

#define TRANSMAXHASH 257
static translation *translation_table[TRANSMAXHASH];
static translation *junkyard;

static const char *translate(const char *key, const char *value)
{
    if (value) {
        int kk = ((key[0] << 5) + key[0]) % TRANSMAXHASH;
        translation *t = translation_table[kk];
        while (t && strcmp(t->key, key) != 0)
            t = t->next;
        if (!t) {
            if (junkyard) {
                t = junkyard;
                junkyard = junkyard->next;
            }
            else {
                t = malloc(sizeof(translation));
                if (!t) abort();
            }
            t->key = str_strdup(key);
            if (!t->key) abort();
            t->value = value;
            t->next = translation_table[kk];
            translation_table[kk] = t;
        }
    }
    return crtag(key);
}

static void report_translations(FILE * F)
{
    int i;
    fputs("TRANSLATION\n", F);
    for (i = 0; i != TRANSMAXHASH; ++i) {
        translation *t = translation_table[i];
        while (t) {
            fprintf(F, "\"%s\";%s\n", t->value, crtag(t->key));
            t = t->next;
        }
    }
}

static void reset_translations(void)
{
    int i;
    for (i = 0; i != TRANSMAXHASH; ++i) {
        translation *t = translation_table[i];
        while (t) {
            translation *c = t->next;
            free(t->key);
            t->next = junkyard;
            junkyard = t;
            t = c;
        }
        translation_table[i] = 0;
    }
}

#include <kernel/objtypes.h>

static void print_items(FILE * F, item * items, const struct locale *lang)
{
    item *itm;

    for (itm = items; itm; itm = itm->next) {
        int in = itm->number;
        const char *ic = resourcename(itm->type->rtype, 0);
        if (itm == items)
            fputs("GEGENSTAENDE\n", F);
        fprintf(F, "%d;%s\n", in, translate(ic, LOC(lang, ic)));
    }
}

static void creport_block(struct stream* out, const char* name)
{
    assert(name);
    sputs(name, out);
}

static void creport_block_1(struct stream* out, const char* name, long i)
{
    assert(name);
    stream_printf(out, "%s %ld\n", name, i);
}

static void creport_block_2(struct stream* out, const char* name, long i, long j)
{
    assert(name);
    stream_printf(out, "%s %ld %ld\n", name, i, j);
}

static void creport_tag(struct stream* out, const char* key, const char* value)
{
    assert(key);
    if (value) {
        stream_printf(out, "\"%s\";%s\n", value, key);
    }
    else {
        stream_printf(out, "\"%s\"\n", key);
    }
}

static void creport_tag_int(struct stream* out, const char* key, long value)
{
    assert(key);
    stream_printf(out, "%ld;%s\n", value, key);
}

static void
cr_output_curses(struct stream *out, const faction * viewer, const void *obj, objtype_t typ)
{
    bool header = false;
    attrib *a = NULL;
    int self = 0;
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
        if (owner != NULL) {
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
        unit *owner = building_owner(b);
        a = b->attribs;
        r = b->region;
        if (owner != NULL) {
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

    while (a) {
        if (a->type == &at_curse) {
            curse *c = (curse *)a->data.v;
            message *msg;

            self = curse_cansee(c, viewer, typ, obj, self);
            msg = msg_curse(c, obj, typ, self);

            if (msg) {
                if (!header) {
                    header = 1;
                    stream_printf(out, "EFFECTS\n");
                }
                nr_render(msg, viewer->locale, g_bigbuf, sizeof(g_bigbuf), viewer);
                stream_printf(out, "\"%s\"\n", g_bigbuf);
                msg_release(msg);
            }
            a = a->next;
        }
        else if (a->type == &at_effect && self) {
            int value = effect_value(a);
            if (value > 0) {
                const struct item_type *itype = effect_type(a);
                const char *key = resourcename(itype->rtype, 0);
                if (!header) {
                    header = 1;
                    stream_printf(out, "EFFECTS\n");
                }
                stream_printf(out, "\"%d %s\"\n", value, translate(key,
                    LOC(viewer->locale, key)));
            }
            a = a->next;
        }
        else {
            a = a->nexttype;
        }
    }
}

static int cr_unit(variant var, const char *name, char *buffer, const void *userdata)
{
    unit *u = (unit *)var.v;
    sprintf(buffer, "%d;%s\n", u ? u->no : -1, name);
    return 0;
}

static int cr_ship(variant var, const char *name, char *buffer, const void *userdata)
{
    ship *u = (ship *)var.v;
    sprintf(buffer, "%d;%s\n", u ? u->no : -1, name);
    return 0;
}

static int cr_building(variant var, const char *name, char *buffer, const void *userdata)
{
    building *u = (building *)var.v;
    sprintf(buffer, "%d;%s\n", u ? u->no : -1, name);
    return 0;
}

static int cr_faction(variant var, const char *name, char *buffer, const void *userdata)
{
    faction *f = (faction *)var.v;
    sprintf(buffer, "%d;%s\n", f ? f->no : -1, name);
    return 0;
}

static int cr_region(variant var, const char *name, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    region *r = (region *)var.v;
    if (r) {
        plane *pl = rplane(r);
        int nx = r->x, ny = r->y;
        pnormalize(&nx, &ny, pl);
        adjust_coordinates(report, &nx, &ny, pl);
        sprintf(buffer, "%d %d %d;%s\n", nx, ny, plane_id(pl), name);
        return 0;
    }
    return -1;
}

static int cr_resource(variant var, const char *name, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const resource_type *r = (const resource_type *)var.v;
    if (r) {
        const char *key = resourcename(r, 0);
        sprintf(buffer, "\"%s\";%s\n",
            translate(key, LOC(report->locale, key)), name);
        return 0;
    }
    return -1;
}

static int cr_race(variant var, const char *name, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const struct race *rc = (const race *)var.v;
    const char *key = rc_name_s(rc, NAME_SINGULAR);
    sprintf(buffer, "\"%s\";%s\n",
        translate(key, LOC(report->locale, key)), name);
    return 0;
}

static int cr_alliance(variant var, const char *name, char *buffer, const void *userdata)
{
    const alliance *al = (const alliance *)var.v;
    UNUSED_ARG(userdata);
    if (al != NULL) {
        sprintf(buffer, "%d;%s\n", al->id, name);
    }
    return 0;
}

static int cr_skill(variant var, const char *name, char *buffer, const void *userdata)
{
    const faction *f = (const faction *)userdata;
    skill_t sk = (skill_t)var.i;
    UNUSED_ARG(userdata);
    if (sk != NOSKILL)
        sprintf(buffer, "\"%s\";%s\n",
            translate(mkname("skill", skillnames[sk]), skillname(sk,
                f->locale)), name);
    else
        sprintf(buffer, "\"\";%s\n", name);
    return 0;
}

static int cr_order(variant var, const char *name, char *buffer, const void *userdata)
{
    order *ord = (order *)var.v;
    const faction *f = (const faction *)userdata;

    if (ord != NULL) {
        char cmd[ORDERSIZE];
        char *wp = buffer;
        const char *rp;

        get_command(ord, f->locale, cmd, sizeof(cmd));

        *wp++ = '\"';
        for (rp = cmd; *rp;) {
            char r = *rp++;
            if (r == '\"' || r == '\\') {
                *wp++ = '\\';
            }
            *wp++ = r;
        }
        sprintf(wp, "\";%s\n", name);
    }
    else
        sprintf(buffer, "\"\";%s\n", name);
    return 0;
}

static int cr_resources(variant var, const char *name, char *buffer, const void *userdata)
{
    faction *f = (faction *)userdata;
    resource *rlist = (resource *)var.v;
    char *wp = buffer;
    if (rlist != NULL) {
        const char *name = resourcename(rlist->type, rlist->number != 1);
        assert(name);
        wp +=
            sprintf(wp, "\"%d %s", rlist->number, translate(name, LOC(f->locale,
                name)));
        for (;;) {
            rlist = rlist->next;
            if (rlist == NULL)
                break;
            name = resourcename(rlist->type, rlist->number != 1);
            assert(name);
            wp +=
                sprintf(wp, ", %d %s", rlist->number, translate(name,
                    LOC(f->locale, name)));
        }
        sprintf(wp, "\";%s\n", name);
    }
    return 0;
}

static int cr_regions(variant var, const char *name, char *buffer, const void *userdata)
{
    faction *f = (faction *)userdata;
    const arg_regions *rdata = (const arg_regions *)var.v;

    if (rdata != NULL && rdata->nregions > 0) {
        region *r = rdata->regions[0];
        plane *pl = rplane(r);
        int i, z = plane_id(pl);
        char *wp = buffer;
        int nx = r->x, ny = r->y;

        pnormalize(&nx, &ny, pl);
        adjust_coordinates(f, &nx, &ny, pl);
        wp += sprintf(wp, "\"%d %d %d", nx, ny, z);
        for (i = 1; i != rdata->nregions; ++i) {
            r = rdata->regions[i];
            pl = rplane(r);
            z = plane_id(pl);
            wp += sprintf(wp, ", %d %d %d", nx, ny, z);
        }
        wp += sprintf(wp, "\";%s\n", name);
    }
    else {
        sprintf(buffer, "\"\";%s\n", name);
    }
    return 0;
}

static int cr_spell(variant var, const char *name, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    spell *sp = (spell *)var.v;
    if (sp != NULL) {
        sprintf(buffer, "\"%s\";%s\n", spell_name(mkname_spell(sp), report->locale), name);
    } 
    else {
        sprintf(buffer, "\"\";%s\n", name);
    }
    return 0;
}

static int cr_curse(variant var, const char *name, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const curse_type *ctype = (const curse_type *)var.v;
    if (ctype != NULL) {
        sprintf(buffer, "\"%s\";%s\n", curse_name(ctype, report->locale), name);
    }
    else
        sprintf(buffer, "\"\";%s\n", name);
    return 0;
}

/*static int msgno; */

static int message_id(const struct message *msg)
{
    variant var;
    var.v = (void *)msg;
    return var.i & 0x7FFFFFFF;
}

/** writes a quoted string to the file
* no trailing space, since this is used to make the creport.
*/
static int write_quoted(stream* out, const char* str)
{
    int nwrite = 0;
    swrite("\"", 1, 1, out);
    if (str) {
        while (*str) {
            char c = (char)*str++;
            switch (c) {
            case '"':
            case '\\':
                swrite("\\", 1, 1, out);
                swrite(&c, 1, 1, out);
                nwrite += 2;
                break;
            case '\n':
                swrite("\\n", 1, 2, out);
                nwrite += 2;
                break;
            default:
                swrite(&c, 1, 1, out);
                ++nwrite;
            }
        }
    }
    swrite("\"", 1, 1, out);
    return nwrite + 2;
}

static void render_messages(stream *out, const faction * f, message_list * msgs)
{
    if (msgs) {
        struct mlist* m = msgs->begin;
        while (m) {
            bool printed = false;
            const struct message_type* mtype = m->msg->type;
            unsigned int hash = mtype->key;
            g_bigbuf[0] = '\0';
            if (nr_render(m->msg, f->locale, g_bigbuf, sizeof(g_bigbuf), f) > 0) {
                creport_block_1(out, "MESSAGE", message_id(m->msg));
                creport_tag_int(out, "type", hash);
                if (m->msg->type->section) {
                    creport_tag(out, "section", m->msg->type->section);
                }
                write_quoted(out, g_bigbuf);
                sputs(";rendered", out);
                printed = true;
            }
            g_bigbuf[0] = '\0';
            if (m->msg->type->nparameters) {
                size_t bytes = cr_render(m->msg, g_bigbuf, (const void*)f);
                if (bytes > 0) {
                    if (g_bigbuf[0]) {
                        if (!printed) {
                            creport_block_1(out, "MESSAGE", message_id(m->msg));
                        }
                        swrite(g_bigbuf, 1, bytes, out);
                    }
                }
                else {
                    log_error("could not render cr-message %p: %s\n", m->msg, m->msg->type->name);
                }
            }
            m = m->next;
        }
    }
}

static void render_messages_compat(FILE* F, message_list * msgs, faction * f)
{
    if (msgs) {
        stream strm;
        fstream_init(&strm, F);
        render_messages(&strm, f, msgs);
    }
}

static void cr_output_battles(FILE * F, faction * f)
{
    struct bmsg *bm;
    for (bm = f->battles; bm; bm = bm->next) {
        region *rb = bm->r;
        plane *pl = rplane(rb);
        int plid = plane_id(pl);
        int nx = rb->x, ny = rb->y;

        pnormalize(&nx, &ny, pl);
        adjust_coordinates(f, &nx, &ny, pl);
        if (!plid)
            fprintf(F, "BATTLE %d %d\n", nx, ny);
        else {
            fprintf(F, "BATTLE %d %d %d\n", nx, ny, plid);
        }
        render_messages_compat(F, bm->msgs, f);
    }
}

/* prints a building */
static void cr_output_building(struct stream *out, const building *b, 
    const unit *owner, int fno, const faction *f)
{
    const char *bname, *billusion;

    stream_printf(out, "BURG %d\n", b->no);

    report_building(b, &bname, &billusion);
    if (billusion) {
        stream_printf(out, "\"%s\";Typ\n", translate(billusion, LOC(f->locale,
            billusion)));
        if (owner && owner->faction == f) {
            stream_printf(out, "\"%s\";wahrerTyp\n", translate(bname, LOC(f->locale,
                bname)));
        }
    }
    else {
        stream_printf(out, "\"%s\";Typ\n", translate(bname, LOC(f->locale, bname)));
    }
    stream_printf(out, "\"%s\";Name\n", b->name);
    if (b->display && b->display[0]) {
        stream_printf(out, "\"%s\";Beschr\n", b->display);
    }
    if (b->size) {
        stream_printf(out, "%d;Groesse\n", b->size);
    }
    if (owner) {
        stream_printf(out, "%d;Besitzer\n", owner->no);
    }
    if (fno >= 0) {
        stream_printf(out, "%d;Partei\n", fno);
    }

    cr_output_curses(out, f, b, TYP_BUILDING);
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints a ship */
static void cr_output_ship(struct stream *out, const ship *sh, const unit *u,
    int fcaptain, const faction *f, const region *r)
{
    int w = 0;
    assert(sh);
    stream_printf(out, "SCHIFF %d\n", sh->no);
    stream_printf(out, "\"%s\";Name\n", sh->name);
    if (sh->display && sh->display[0])
        stream_printf(out, "\"%s\";Beschr\n", sh->display);
    stream_printf(out, "\"%s\";Typ\n", translate(sh->type->_name,
        LOC(f->locale, sh->type->_name)));
    stream_printf(out, "%d;Anzahl\n", sh->number);
    stream_printf(out, "%d;Groesse\n", sh->size);
    if (sh->damage) {
        int percent = ship_damage_percent(sh);
        stream_printf(out, "%d;Schaden\n", percent);
    }
    if (u) {
        stream_printf(out, "%d;Kapitaen\n", u->no);
    }
    if (fcaptain >= 0) {
        stream_printf(out, "%d;Partei\n", fcaptain);
    }
    /* calculate cargo */
    if (u && (u->faction == f || omniscient(f))) {
        int n = 0, p = 0;
        int mweight = ship_capacity(sh);
        getshipweight(sh, &n, &p);

        stream_printf(out, "%d;capacity\n", mweight);
        stream_printf(out, "%d;cargo\n", n);
        stream_printf(out, "%d;speed\n", shipspeed(sh, u));
    }
    /* shore */
    w = NODIRECTION;
    if (!fval(r->terrain, SEA_REGION))
        w = sh->coast;
    if (w != NODIRECTION)
        stream_printf(out, "%d;Kueste\n", w);

    cr_output_curses(out, f, sh, TYP_SHIP);
}

static void cr_output_spells(stream *out, const unit * u, int maxlevel)
{
    spellbook * book = unit_get_spellbook(u);

    if (book) {
        const faction * f = u->faction;
        selist *ql;
        int qi, header = 0;

        for (ql = book->spells, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
            spellbook_entry * sbe = (spellbook_entry *)selist_get(ql, qi);
            if (sbe->level <= maxlevel) {
                const spell *sp = spellref_get(&sbe->spref);
                const char * spname = mkname("spell", sp->sname);
                const char *name = translate(spname, spell_name(spname, f->locale));
                if (!header) {
                    stream_printf(out, "SPRUECHE\n");
                    header = 1;
                }
                stream_printf(out, "\"%s\"\n", name);
            }
        }
    }
}

int level_days(unsigned int level)
{
    /* FIXME STUDYDAYS * ((level + 1) * level / 2); */
    return 30 * ((level + 1) * level / 2);
}

/** prints all that belongs to a unit
* @param f observers faction
* @param u unit to report
*/
void cr_output_unit(struct stream *out, const struct faction * f,
    const struct unit * u, enum seen_mode mode)
{
    /* Race attributes are always plural and item attributes always
     * singular */
    const char *str;
    int pr;
    item *itm, *show = NULL;
    const char *pzTmp;
    item result[MAX_INVENTORY];
    const faction *fother;
    const char *prefix;
    bool allied;
    const struct locale *lang = f->locale;

    assert(u && u->number);

    stream_printf(out, "EINHEIT %d\n", u->no);
    stream_printf(out, "\"%s\";Name\n", unit_getname(u));
    str = u_description(u, lang);
    if (str) {
        stream_printf(out, "\"%s\";Beschr\n", str);
    }

    if (u->faction == f) {
        unit *mage;
        group * g;

        g = get_group(u);
        if (g) {
            stream_printf(out, "%d;gruppe\n", g->gid);
        }
        mage = get_familiar_mage(u);
        if (mage) {
            stream_printf(out, "%d;familiarmage\n", mage->no);
        }
    }

    fother = get_otherfaction(u);
    allied = u->faction == f || alliedunit(u, f, HELP_FSTEALTH);
    if (allied) {
        /* allies can tell that the unit is anonymous */
        /* the true faction is visible to allies */
        stream_printf(out, "%d;Partei\n", u->faction->no);
        if (fother) {
            stream_printf(out, "%d;Anderepartei\n", fother->no);
        }
    }
    else if (!fval(u, UFL_ANON_FACTION)) {
        /* OBS: anonymity overrides everything */
        /* we have no alliance, so we get duped */
        stream_printf(out, "%d;Partei\n", (fother ? fother : u->faction)->no);
        if (fother == f) {
            /* sieht aus wie unsere, ist es aber nicht. */
            stream_printf(out, "1;Verraeter\n");
        }
    }
    if (fval(u, UFL_ANON_FACTION)) {
        sputs("1;Parteitarnung", out);
    }
    prefix = raceprefix(u);
    if (prefix) {
        prefix = mkname("prefix", prefix);
        stream_printf(out, "\"%s\";typprefix\n", translate(prefix, LOC(lang,
            prefix)));
    }
    stream_printf(out, "%d;Anzahl\n", u->number);

    pzTmp = get_racename(u->attribs);
    if (pzTmp) {
        const race *irace = rc_find(pzTmp);
        if (irace) {
            const char *pzRace = rc_name_s(irace, NAME_PLURAL);
            stream_printf(out, "\"%s\";Typ\n",
                translate(pzRace, LOC(lang, pzRace)));
        }
        else {
            stream_printf(out, "\"%s\";Typ\n", pzTmp);
        }
    }
    else {
        const race *irace = u_irace(u);
        const char *zRace = rc_name_s(irace, NAME_PLURAL);
        stream_printf(out, "\"%s\";Typ\n",
            translate(zRace, LOC(lang, zRace)));
        if (u->faction == f && irace != u_race(u)) {
            zRace = rc_name_s(u_race(u), NAME_PLURAL);
            stream_printf(out, "\"%s\";wahrerTyp\n",
                translate(zRace, LOC(lang, zRace)));
        }
    }

    if (u->building) {
        assert(u->building->region);
        stream_printf(out, "%d;Burg\n", u->building->no);
    }
    if (u->ship) {
        assert(u->ship->region);
        stream_printf(out, "%d;Schiff\n", u->ship->no);
    }
    if (is_guard(u)) {
        stream_printf(out, "%d;bewacht\n", 1);
    }
    /* additional information for own units */
    if (u->faction == f || omniscient(f)) {
        order *ord;
        const char *xc;
        const char *c;
        int i;
        struct sc_mage *mage;
        ptrdiff_t s, len;

        i = ualias(u);
        if (i > 0)
            stream_printf(out, "%d;temp\n", i);
        else if (i < 0)
            stream_printf(out, "%d;alias\n", -i);
        i = get_money(u);
        stream_printf(out, "%d;Kampfstatus\n", u->status);
        stream_printf(out, "%d;weight\n", weight(u));
        if (fval(u, UFL_NOAID)) {
            stream_printf(out, "1;unaided\n");
        }
        if (fval(u, UFL_STEALTH)) {
            i = u_geteffstealth(u);
            if (i >= 0) {
                stream_printf(out, "%d;Tarnung\n", i);
            }
        }
        xc = uprivate(u);
        if (xc) {
            stream_printf(out, "\"%s\";privat\n", xc);
        }
        c = hp_status(u);
        if (c && *c && (u->faction == f || omniscient(f))) {
            stream_printf(out, "\"%s\";hp\n", translate(c,
                LOC(u->faction->locale, c)));
        }
        if (fval(u, UFL_HERO)) {
            stream_printf(out, "1;hero\n");
        }

        if (fval(u, UFL_HUNGER) && (u->faction == f)) {
            stream_printf(out, "1;hunger\n");
        }
        mage = get_mage(u);
        if (mage) {
            stream_printf(out, "%d;Aura\n", get_spellpoints(u));
            stream_printf(out, "%d;Auramax\n", max_spellpoints(u, u->region));
        }
        /* default commands */
        stream_printf(out, "COMMANDS\n");
        for (ord = u->orders; ord; ord = ord->next) {
            swrite("\"", 1, 1, out);
            stream_order(out, ord, lang, true);
            swrite("\"\n", 1, 2, out);
        }

        /* talents */
        pr = 0;
        for (len = arrlen(u->skills), s = 0; s != len; ++s) {
            skill* sv = u->skills + s;
            if (sv->level > 0) {
                skill_t sk = sv->id;
                int esk = effskill(u, sk, NULL);
                if (!pr) {
                    pr = 1;
                    stream_printf(out, "TALENTE\n");
                }
                stream_printf(out, "%d %d;%s\n", u->number * level_days(sv->level), esk,
                    translate(mkname("skill", skillnames[sk]), skillname(sk,
                        lang)));
            }
        }

        /* spells that this unit can cast */
        if (mage) {
            int maxlevel = effskill(u, SK_MAGIC, NULL);
            cr_output_spells(out, u, maxlevel);

            for (i = 0; i != MAXCOMBATSPELLS; ++i) {
                int level;
                const spell *sp = mage_get_combatspell(mage, i, &level);
                if (sp) {
                    const char *name = mkname_spell(sp);
                    if (level > maxlevel) {
                        level = maxlevel;
                    }
                    stream_printf(out, "KAMPFZAUBER %d\n", i);
                    name = translate(name, spell_name(name, lang));
                    stream_printf(out, "\"%s\";name\n", name);
                    stream_printf(out, "%d;level\n", level);
                }
            }
        }
    }
    /* items */
    pr = 0;
    if (f == u->faction || omniscient(f)) {
        show = u->items;
    }
    else {
        if (mode >= seen_unit) {
            int n = report_items(u, result, MAX_INVENTORY, u, f);
            assert(n >= 0);
            if (n > 0) {
                show = result;
            }
        }
    }
    for (itm = show; itm; itm = itm->next) {
        const char *ic;
        int in;
        report_item(u, itm, f, NULL, &ic, &in, true);
        if (in == 0)
            continue;
        if (!pr) {
            pr = 1;
            stream_printf(out, "GEGENSTAENDE\n");
        }
        stream_printf(out, "%d;%s\n", in, translate(ic, LOC(lang, ic)));
    }

    cr_output_curses(out, f, u, TYP_UNIT);
}

static void print_ally(const faction *f, faction *af, int status, FILE *F) {
    if (af && status > 0) {
        fprintf(F, "ALLIANZ %d\n", af->no);
        fprintf(F, "\"%s\";Parteiname\n", af->name);
        fprintf(F, "%d;Status\n", status & HELP_ALL);
    }
}

struct print_ally_s {
    const faction *f;
    FILE *F;
};

static int print_ally_cb(struct allies *al, faction *af, int status, void *udata) {
    UNUSED_ARG(al);
    if (af && faction_alive(af)) {
        struct print_ally_s *data = (struct print_ally_s *)udata;
        int mode = alliance_status(data->f, af, status);
        print_ally(data->f, af, mode, data->F);
    }
    return 0;
}

/* prints allies */
static void show_allies_cr(FILE * F, const faction * f, const group *g)
{
    struct print_ally_s data;
    data.F = F;
    data.f = f;
    struct allies *sf = g ? g->allies : f->allies;
    allies_walk(sf, print_ally_cb, &data);
}

/* prints allies */
static void show_alliances_cr(FILE * F, const faction * f)
{
    alliance *al = f_get_alliance(f);
    if (al) {
        faction *lead = alliance_get_leader(al);
        assert(lead);
        fprintf(F, "ALLIANCE %d\n", al->id);
        fprintf(F, "\"%s\";name\n", al->name);
        fprintf(F, "%d;leader\n", lead->no);
    }
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* this is a copy of laws.c->find_address output changed. */
static void cr_find_address(FILE * F, const faction * uf, selist * addresses)
{
    int i = 0;
    selist *flist = addresses;
    while (flist) {
        const faction *f = (const faction *)selist_get(flist, i);
        if (uf != f) {
            const char *str;
            fprintf(F, "PARTEI %d\n", f->no);
            fprintf(F, "\"%s\";Parteiname\n", f->name);
            if (strcmp(faction_getemail(f), "") != 0)
                fprintf(F, "\"%s\";email\n", faction_getemail(f));
            str = faction_getbanner(f);
            if (str) {
                fprintf(F, "\"%s\";banner\n", str);
            }
            fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
            if (f->alliance && f->alliance == uf->alliance) {
                fprintf(F, "%d;alliance\n", f->alliance->id);
            }
        }
        selist_advance(&flist, &i, 1);
    }
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

static void cr_reportspell(FILE * F, const spell * sp, int level, const struct locale *lang)
{
    int k;
    const char *spname = mkname_spell(sp);
    const char *name = translate(spname, spell_name(spname, lang));

    fprintf(F, "ZAUBER %d\n", str_hash(sp->sname));
    fprintf(F, "\"%s\";name\n", name);
    fprintf(F, "%d;level\n", level);
    fprintf(F, "%d;rank\n", sp->rank);
    fprintf(F, "\"%s\";info\n", spell_info(sp, lang));
    if (sp->parameter)
        fprintf(F, "\"%s\";syntax\n", sp->parameter);
    else
        fputs("\"\";syntax\n", F);

    if (sp->sptyp & PRECOMBATSPELL)
        fputs("\"precombat\";class\n", F);
    else if (sp->sptyp & COMBATSPELL)
        fputs("\"combat\";class\n", F);
    else if (sp->sptyp & POSTCOMBATSPELL)
        fputs("\"postcombat\";class\n", F);
    else
        fputs("\"normal\";class\n", F);

    if (sp->sptyp & FARCASTING)
        fputs("1;far\n", F);
    if (sp->sptyp & OCEANCASTABLE)
        fputs("1;ocean\n", F);
    if (sp->sptyp & ONSHIPCAST)
        fputs("1;ship\n", F);
    if (!(sp->sptyp & NOTFAMILIARCAST))
        fputs("1;familiar\n", F);
    fputs("KOMPONENTEN\n", F);

    for (k = 0; sp->components[k].type; ++k) {
        const resource_type *rtype = sp->components[k].type;
        int itemanz = sp->components[k].amount;
        int costtyp = sp->components[k].cost;
        if (itemanz > 0) {
            name = resourcename(rtype, 0);
            fprintf(F, "%d %d;%s\n", itemanz, costtyp == SPC_LEVEL
                || costtyp == SPC_LINEAR, translate(name, LOC(lang, name)));
        }
    }
}

static char *cr_output_resource(char *buf, const resource_type *rtype,
    const struct locale *loc, int amount, int level)
{
    const char *name, *tname;
    assert(rtype);
    name = resourcename(rtype, NMF_PLURAL);
    assert(name);
    buf += sprintf(buf, "RESOURCE %d\n", str_hash(rtype->_name));
    tname = LOC(loc, name);
    assert(tname);
    tname = translate(name, tname);
    assert(tname);
    buf += sprintf(buf, "\"%s\";type\n", tname);
    if (amount >= 0) {
        if (level >= 0)
            buf += sprintf(buf, "%d;skill\n", level);
        buf += sprintf(buf, "%d;number\n", amount);
    }
    return buf;
}

static void
cr_borders(stream *out, const region * r, const faction * f)
{
    direction_t d;
    int g = 0;
    for (d = 0; d != MAXDIRECTIONS; d++) {        /* Nachbarregionen, die gesehen werden, ermitteln */
        const region *r2 = rconnect(r, d);
        const connection *b;
        if (!r2)
            continue;
        b = get_borders(r, r2);
        while (b) {
            bool cs = b->type->fvisible(b, f, r);

            if (!cs) {
                cs = b->type->rvisible(b, r);
                if (!cs) {
                    unit *us = r->units;
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
            if (cs) {
                creport_block_1(out, "GRENZE", ++g);
                creport_tag(out, "typ", border_name(b, r, f, GF_PURE, f->locale));
                creport_tag_int(out, "richtung", d);
                if (!b->type->transparent(b, f))
                    sputs("1;opaque", out);
                /* hack: */
                if (b->type == &bt_road && r->terrain->max_road) {
                    int p = rroad(r, d) * 100 / r->terrain->max_road;
                    creport_tag_int(out, "prozent", p);
                }
            }
            b = b->next;
        }
    }
}

void cr_output_resources(struct stream *out, const struct faction * f, const struct region *r, enum seen_mode mode)
{
    char *pos = g_bigbuf;
    resource_report result[MAX_RAWMATERIALS];
    int n, size = report_resources(r, result, f, mode);

#ifdef RESOURCECOMPAT
    int trees = rtrees(r, 2);
    int saplings = rtrees(r, 1);

    if (trees > 0) {
        stream_printf(out, "%d;Baeume\n", trees);
    }
    if (saplings > 0) {
        stream_printf(out, "%d;Schoesslinge\n", saplings);
    }
    if (fval(r, RF_MALLORN) && (trees > 0 || saplings > 0)) {
        sputs("1;Mallorn", out);
    }
    for (n = 0; n < size; ++n) {
        if (result[n].level >= 0 && result[n].number >= 0) {
            const char * name = resourcename(result[n].rtype, 1);
            assert(name);
            stream_printf(out, "%d;%s\n", result[n].number, crtag(name));
        }
    }
#endif

    for (n = 0; n < size; ++n) {
        if (result[n].number >= 0) {
            pos =
                cr_output_resource(pos, result[n].rtype, f->locale, result[n].number,
                    result[n].level);
        }
    }
    if (pos != g_bigbuf) {
        swrite(g_bigbuf, 1, pos - g_bigbuf, out);
    }
}

static void
cr_region_header(struct stream *out, int plid, int nx, int ny, int uid)
{
    char buf[64];
    if (plid == 0) {
        snprintf(buf, sizeof(buf), "REGION %d %d", nx, ny);
    }
    else {
        snprintf(buf, sizeof(buf), "REGION %d %d %d", nx, ny, plid);
    }
    sputs(buf, out);
    if (uid) {
        snprintf(buf, sizeof(buf), "%d;id", uid);
        sputs(buf, out);
    }
}

typedef struct travel_data {
    const faction *f;
    struct stream *out;
    int n;
} travel_data;

static void cb_cr_travelthru_ship(region *r, const unit *u, void *cbdata) {
    travel_data *data = (travel_data *)cbdata;
    const faction *f = data->f;
    stream *out = data->out;

    if (u->ship && travelthru_cansee(r, f, u)) {
        if (data->n++ == 0) {
            creport_block(out, "DURCHSCHIFFUNG");
        }
        creport_tag(out, shipname(u->ship), NULL);
    }
}

static void cb_cr_travelthru_unit(region *r, const unit *u, void *cbdata) {
    travel_data *data = (travel_data *)cbdata;
    const faction *f = data->f;
    stream *out = data->out;

    if (!u->ship && travelthru_cansee(r, f, u)) {
        if (data->n++ == 0) {
            creport_block(out, "DURCHREISE");
        }
        creport_tag(out, unitname(u), NULL);
    }
}

static void cr_output_travelthru(stream *out, region *r, const faction *f) {
    /* describe both passed and inhabited regions */
    travel_data cbdata = { 0 };
    cbdata.f = f;
    cbdata.out = out;
    cbdata.n = 0;
    travelthru_map(r, cb_cr_travelthru_ship, &cbdata);
    cbdata.n = 0;
    travelthru_map(r, cb_cr_travelthru_unit, &cbdata);
}

static void cr_output_region_compat(FILE* F, report_context* ctx, region* r)
{
    /* TODO: eliminate this function */
    stream strm;
    fstream_init(&strm, F);
    cr_output_region(&strm, ctx->f, r, r->seen.mode);
}

static void cb_output_price(struct demand *dmd, int n, void *data)
{
    output_context *ctx = (output_context *)data;
    struct stream *out = ctx->out;
    const faction *f = ctx->f;
    int i;
    for (i = 0; i != n; ++i) {
        const char *ch = resourcename(dmd[i].type->itype->rtype, 0);
        creport_tag_int(out, translate(ch, LOC(f->locale, ch)), (dmd[i].value
            ? dmd[i].value * dmd[i].type->price
            : -dmd[i].type->price));
    }
}

void cr_output_region(struct stream* out, const struct faction* f,
    struct region* r, enum seen_mode mode)
{
    plane *pl = rplane(r);
    int plid = plane_id(pl), nx, ny;
    const char *tname;
    int oc[4][2], o = 0;
    int uid = r->uid;

    if (opt_cr_absolute_coords) {
        nx = r->x;
        ny = r->y;
    }
    else {
        nx = r->x, ny = r->y;
        pnormalize(&nx, &ny, pl);
        adjust_coordinates(f, &nx, &ny, pl);
    }

    if (pl) {
        if (ny == pl->maxy) {
            oc[o][0] = nx;
            oc[o++][1] = pl->miny - 1;
        }
        if (nx == pl->maxx) {
            oc[o][0] = pl->minx - 1;
            oc[o++][1] = ny;
        }
        if (ny == pl->miny) {
            oc[o][0] = nx;
            oc[o++][1] = pl->maxy + 1;
        }
        if (nx == pl->minx) {
            oc[o][0] = pl->maxx + 1;
            oc[o++][1] = ny;
        }
    }
    while (o--) {
        cr_region_header(out, plid, oc[o][0], oc[o][1], uid);
        sputs("\"wrap\";visibility", out);
    }

    cr_region_header(out, plid, nx, ny, uid);

    if (r->land) {
        const char *str = rname(r, f->locale);
        if (str && str[0]) {
            creport_tag(out, "Name", str);
        }
    }
    tname = terrain_name(r);

    creport_tag(out, "Terrain", translate(tname, LOC(f->locale, tname)));
    if (mode != seen_unit) {
        creport_tag(out, "visibility", visibility[mode]);
    }
    if (mode > seen_neighbour) {
        if (r->land && r->land->display && r->land->display[0]) {
            creport_tag(out, "Beschr", r->land->display);
        }
        if (r->land) {
            creport_tag_int(out, "Bauern", rpeasants(r));

            if (mode >= seen_travel) {
                creport_tag_int(out, "Pferde", rhorses(r));
                creport_tag_int(out, "Silber", rmoney(r));
                if (skill_enabled(SK_ENTERTAINMENT)) {
                    creport_tag_int(out, "Unterh", entertainmoney(r));
                }
                creport_tag_int(out, "Rekruten", max_recruits(r));
                if (max_production(r)) {
                    /* Im CR steht der Bauernlohn, der bei Trauer nur 10 ist */
                    bool mourn = is_mourning(r, turn);
                    int p_wage = peasant_wage(r, mourn);
                    creport_tag_int(out, "Lohn", p_wage);
                    if (mourn) {
                        sputs("1;mourning", out);
                    }
                }
                if (rule_region_owners()) {
                    faction *owner = region_get_owner(r);
                    if (owner) {
                        creport_tag_int(out, "owner", owner->no);
                    }
                }
                if (r->land && r->land->ownership) {
                    creport_tag_int(out, "morale", region_get_morale(r));
                }
            }

            /* this writes both some tags (RESOURCECOMPAT) and a block.
             * must not write any blocks before it */
            cr_output_resources(out, f, r, mode);

            if (mode >= seen_unit) {
                /* trade */
                if (markets_module() && r->land) {
                    const item_type *lux = r_luxury(r);
                    const item_type *herb = r->land->herbtype;
                    if (lux || herb) {
                        creport_block(out, "PREISE");
                        if (lux) {
                            const char *ch = resourcename(lux->rtype, 0);
                            creport_tag_int(out, translate(ch, LOC(f->locale, ch)), 1);
                        }
                        if (herb) {
                            const char *ch = resourcename(herb->rtype, 0);
                            creport_tag_int(out, translate(ch, LOC(f->locale, ch)), 1);
                        }
                    }
                }
                else if (rpeasants(r) / TRADE_FRACTION > 0) {
                    if (r_has_demands(r)) {
                        output_context ctx = {f, out };
                        creport_block(out, "PREISE");
                        r_foreach_demand(r, cb_output_price, &ctx);
                    }
                }
            }
        }
        cr_output_curses(out, f, r, TYP_REGION);
        cr_borders(out, r, f);
    }
    if (see_schemes(r, mode)) {
        /* Sonderbehandlung Teleport-Ebene */
        region *rl[MAX_SCHEMES];
        int num = get_astralregions(r, inhabitable, rl);

        if (num > 0) {
            int i;
            for (i = 0; i != num; ++i) {
                region *r2 = rl[i];
                plane *plx = rplane(r2);

                nx = r2->x;
                ny = r2->y;
                pnormalize(&nx, &ny, plx);
                adjust_coordinates(f, &nx, &ny, plx);
                creport_block_2(out, "SCHEMEN", nx, ny);
                creport_tag(out, "Name", rname(r2, f->locale));
            }
        }
    }
    if (mode >= seen_travel) {
        message_list *mlist = r_getmessages(r, f);
        render_messages(out, f, r->msgs);
        if (mlist) {
            render_messages(out, f, mlist);
        }
    }
    if (mode >= seen_lighthouse) {
        cr_output_travelthru(out, r, f);

        /* buildings */
        if (r->buildings) {
            building *b;
            for (b = rbuildings(r); b; b = b->next) {
                int fno = -1;
                unit *u = building_owner(b);
                if (u && !fval(u, UFL_ANON_FACTION)) {
                    const faction *sf = visible_faction(f, u, get_otherfaction(u));
                    fno = sf->no;
                }
                cr_output_building(out, b, u, fno, f);
            }
        }

        /* ships */
        if (r->ships) {
            ship *sh;
            for (sh = r->ships; sh; sh = sh->next) {
                int fno = -1;
                unit *u = ship_owner(sh);
                if (u && !fval(u, UFL_ANON_FACTION)) {
                    const faction *sf = visible_faction(f, u, get_otherfaction(u));
                    fno = sf->no;
                }

                cr_output_ship(out, sh, u, fno, f, r);
            }
        }

        /* visible units */
        if (r->units) {
            unit *u;
            int stealthmod = stealth_modifier(r, f, mode);
            for (u = r->units; u; u = u->next) {
                if (visible_unit(u, f, stealthmod, mode)) {
                    cr_output_unit(out, f, u, mode);
                }
            }
        }
    }
}

static void report_itemtype(FILE *F, faction *f, const item_type *itype) {
    const char *ch;
    const char *description = NULL;
    const char *pname = resourcename(itype->rtype, 0);
    const char *potiontext = mkname("describe", pname);
    
    ch = resourcename(itype->rtype, 0);
    fprintf(F, "TRANK %d\n", str_hash(ch));
    fprintf(F, "\"%s\";Name\n", translate(ch, LOC(f->locale, ch)));
    fprintf(F, "%d;Stufe\n", potion_level(itype));
    description = LOC(f->locale, potiontext);
    fprintf(F, "\"%s\";Beschr\n", description);
    if (itype->construction) {
        requirement *m = itype->construction->materials;
        
        fprintf(F, "ZUTATEN\n");
        
        while (m->number) {
            ch = resourcename(m->rtype, 0);
            fprintf(F, "\"%s\"\n", translate(ch, LOC(f->locale, ch)));
            m++;
        }
    }
}
    
/* main function of the creport. creates the header and traverses all regions */
static int
report_computer(const char *filename, report_context * ctx, const char *bom)
{
    static int era = -1;
    int i, age;
    faction *f = ctx->f;
    const char *prefix, *str;
    region *r;
    const char *mailto = config_get("game.email");
    const attrib *a;
    FILE *F = fopen(filename, "w");
    static const race *rc_human;
    static int rc_cache;

    if (era < 0) {
        era = config_get_int("game.era", 1);
    }
    if (F == NULL) {
        perror(filename);
        return -1;
    }
    else if (bom) {
        fwrite(bom, 1, strlen(bom), F);
    }

    /* must call this to get all the neighbour regions */
    /* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
    /* initialisations, header and lists */

    fprintf(F, "VERSION %d\n", C_REPORT_VERSION);
    fprintf(F, "\"UTF-8\";charset\n\"%s\";locale\n",
        locale_name(f->locale));
    fprintf(F, "%d;noskillpoints\n", 1);
    fprintf(F, "%lld;date\n", (long long)ctx->report_time);
    fprintf(F, "\"%s\";Spiel\n", game_name());
    fprintf(F, "\"%s\";Konfiguration\n", "Standard");
    fprintf(F, "\"%s\";Koordinaten\n", "Hex");
    fprintf(F, "%d;max_units\n", rule_faction_limit());
    fprintf(F, "%d;Basis\n", 36);
    fprintf(F, "%d;Runde\n", turn);
    fprintf(F, "%d;Zeitalter\n", era);
    fprintf(F, "\"%s\";Build\n", eressea_version());
    if (mailto != NULL) {
        const char * mailcmd = get_mailcmd(f->locale);
        fprintf(F, "\"%s\";mailto\n", mailto);
        fprintf(F, "\"%s\";mailcmd\n", mailcmd);
    }

    show_alliances_cr(F, f);

    fprintf(F, "PARTEI %d\n", f->no);
    fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
    if (f_get_alliance(f)) {
        fprintf(F, "%d;alliance\n", f->alliance->id);
        fprintf(F, "%d;joined\n", f->alliance_joindate);
    }
    age = faction_age(f);
    fprintf(F, "%d;age\n", age);
    fprintf(F, "%d;Optionen\n", f->options);
    if (f->options & WANT_OPTION(O_SCORE) && age > DISPLAYSCORE) {
        char score[32];
        write_score(score, sizeof(score), f->score);
        fprintf(F, "%s;Punkte\n", score);
        write_score(score, sizeof(score), average_score_of_age(age));
        fprintf(F, "%s;Punktedurchschnitt\n", score);
    }
    {
        const char *zRace = rc_name_s(f->race, NAME_PLURAL);
        fprintf(F, "\"%s\";Typ\n", translate(zRace, LOC(f->locale, zRace)));
    }
    prefix = get_prefix(f->attribs);
    if (prefix != NULL) {
        prefix = mkname("prefix", prefix);
        fprintf(F, "\"%s\";typprefix\n",
            translate(prefix, LOC(f->locale, prefix)));
    }
    fprintf(F, "%d;Rekrutierungskosten\n", f->race->recruitcost);
    fprintf(F, "%d;Anzahl Personen\n", f->num_people);
    fprintf(F, "\"%s\";Magiegebiet\n", magic_school[f->magiegebiet]);

    if (rc_changed(&rc_cache)) {
        rc_human = rc_find("human");
    }
    if (f->race == rc_human) {
        fprintf(F, "%d;Anzahl Immigranten\n", count_migrants(f));
        fprintf(F, "%d;Max. Immigranten\n", count_maxmigrants(f));
    }

    i = countheroes(f);
    if (i > 0)
        fprintf(F, "%d;heroes\n", i);
    i = max_heroes(f->num_people);
    if (i > 0)
        fprintf(F, "%d;max_heroes\n", i);

    if (faction_age(f) > 1 && f->lastorders != turn - 1) {
        fprintf(F, "%d;nmr\n", turn - 1 - f->lastorders);
    }

    fprintf(F, "\"%s\";Parteiname\n", f->name);
    fprintf(F, "\"%s\";email\n", faction_getemail(f));
    str = faction_getbanner(f);
    if (str) {
        fprintf(F, "\"%s\";banner\n", str);
    }
    print_items(F, f->items, f->locale);
    fputs("OPTIONEN\n", F);
    for (i = 0; i != MAXOPTIONS; ++i) {
        int flag = WANT_OPTION(i);
        if (options[i]) {
            fprintf(F, "%d;%s\n", (f->options & flag) ? 1 : 0, options[i]);
        }
        else if (f->options & flag) {
            f->options &= (~flag);
        }
    }
    show_allies_cr(F, f, NULL);
    {
        group *g;
        for (g = f->groups; g; g = g->next) {

            fprintf(F, "GRUPPE %d\n", g->gid);
            fprintf(F, "\"%s\";name\n", g->name);
            prefix = get_prefix(g->attribs);
            if (prefix != NULL) {
                prefix = mkname("prefix", prefix);
                fprintf(F, "\"%s\";typprefix\n",
                    translate(prefix, LOC(f->locale, prefix)));
            }
            show_allies_cr(F, f, g);
        }
    }

    render_messages_compat(F, f->msgs, f);
    cr_output_battles(F, f);

    cr_find_address(F, f, ctx->addresses);
    a = a_find(f->attribs, &at_reportspell);
    while (a && a->type == &at_reportspell) {
        spellbook_entry *sbe = (spellbook_entry *)a->data.v;
        const spell *sp = spellref_get(&sbe->spref);
        cr_reportspell(F, sp, sbe->level, f->locale);
        a = a->next;
    }
    for (a = a_find(f->attribs, &at_showitem); a && a->type == &at_showitem;
        a = a->next) {
        const item_type *itype = (const item_type *)a->data.v;

        if (itype) {
            report_itemtype(F, f, itype);
        }
    }

    /* traverse all regions */
    for (r = ctx->first; r != ctx->last; r = r->next) {
        if (r->seen.mode > seen_none) {
            cr_output_region_compat(F, ctx, r);
        }
    }
    if (f->locale != crtag_locale()) {
        report_translations(F);
    }
    reset_translations();
    fclose(F);
    return 0;
}

int crwritemap(const char *filename)
{
    FILE *F = fopen(filename, "w");
    region *r;

    if (F) {
        fprintf(F, "VERSION %d\n", C_REPORT_VERSION);
        fputs("\"UTF-8\";charset\n", F);

        for (r = regions; r; r = r->next) {
            plane *pl = rplane(r);
            int plid = plane_id(pl);
            if (plid) {
                fprintf(F, "REGION %d %d %d\n", r->x, r->y, plid);
            }
            else {
                fprintf(F, "REGION %d %d\n", r->x, r->y);
            }
            fprintf(F, "\"%s\";Name\n\"%s\";Terrain\n", rname(r, default_locale),
                LOC(default_locale, terrain_name(r)));
        }
        fclose(F);
        return 0;
    }
    return EOF;
}

void register_cr(void)
{
    tsf_register("report", cr_ignore);
    tsf_register("string", cr_string);
    tsf_register("order", cr_order);
    tsf_register("spell", cr_spell);
    tsf_register("curse", cr_curse);
    tsf_register("int", cr_int);
    tsf_register("unit", cr_unit);
    tsf_register("region", cr_region);
    tsf_register("faction", cr_faction);
    tsf_register("ship", cr_ship);
    tsf_register("building", cr_building);
    tsf_register("skill", cr_skill);
    tsf_register("resource", cr_resource);
    tsf_register("race", cr_race);
    tsf_register("direction", cr_int);
    tsf_register("alliance", cr_alliance);
    tsf_register("resources", cr_resources);
    tsf_register("items", cr_resources);
    tsf_register("regions", cr_regions);

    if (!nocr)
        register_reporttype("cr", &report_computer, 1 << O_COMPUTER);
}

void creport_cleanup(void)
{
    while (junkyard) {
        translation *t = junkyard;
        junkyard = junkyard->next;
        free(t);
    }
    junkyard = 0;
}
