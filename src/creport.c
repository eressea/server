/*
+-------------------+  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
+-------------------+
This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>
#include <kernel/version.h>
#include "creport.h"
#include "travelthru.h"

/* tweakable features */
#define RENDER_CRMESSAGES
#define BUFFERSIZE 32768
#define RESOURCECOMPAT

/* modules include */
#include <modules/score.h>

/* attributes include */
#include <attributes/follow.h>
#include <attributes/orcification.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/raceprefix.h>
#include <attributes/stealth.h>

/* gamecode includes */
#include "laws.h"
#include "economy.h"
#include "move.h"
#include "reports.h"
#include "alchemy.h"
#include "teleport.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/ally.h>
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
#include <kernel/save.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/crmessage.h>
#include <util/strings.h>
#include <util/language.h>
#include <util/log.h>
#include <util/message.h>
#include <util/nrmessage.h>
#include <quicklist.h>
#include <filestream.h>

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
#define C_REPORT_VERSION 66

#define TAG_LOCALE "de"
#ifdef TAG_LOCALE
static const char *crtag(const char *key)
{
    static const struct locale *lang = NULL;
    if (!lang)
        lang = get_locale(TAG_LOCALE);
    return LOC(lang, key);
}
#else
#define crtag(x) (x)
#endif
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
    int kk = ((key[0] << 5) + key[0]) % TRANSMAXHASH;
    translation *t = translation_table[kk];
    while (t && strcmp(t->key, key) != 0)
        t = t->next;
    if (!t) {
        if (junkyard) {
            t = junkyard;
            junkyard = junkyard->next;
        }
        else
            t = malloc(sizeof(translation));
        t->key = _strdup(key);
        t->value = value;
        t->next = translation_table[kk];
        translation_table[kk] = t;
    }
    return crtag(key);
}

static void write_translations(FILE * F)
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

static void
cr_output_curses(stream *out, const faction * viewer, const void *obj, objtype_t typ)
{
    bool header = false;
    attrib *a = NULL;
    int self = 0;
    region *r;

    /* Die Sichtbarkeit eines Zaubers und die Zaubermeldung sind bei
     * Gebäuden und Schiffen je nach, ob man Besitzer ist, verschieden.
     * Bei Einheiten sieht man Wirkungen auf eigene Einheiten immer.
     * Spezialfälle (besonderes Talent, verursachender Magier usw. werde
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
        if (fval(a->type, ATF_CURSE)) {
            curse *c = (curse *)a->data.v;
            message *msg;

            self = curse_cansee(c, viewer, typ, obj, self);
            msg = msg_curse(c, obj, typ, self);

            if (msg) {
                char buf[BUFFERSIZE];
                if (!header) {
                    header = 1;
                    stream_printf(out, "EFFECTS\n");
                }
                nr_render(msg, viewer->locale, buf, sizeof(buf), viewer);
                stream_printf(out, "\"%s\"\n", buf);
                msg_release(msg);
            }
            a = a->next;
        }
        else if (a->type == &at_effect && self) {
            effect_data *data = (effect_data *)a->data.v;
            if (data->value > 0) {
                const char *key = resourcename(data->type->itype->rtype, 0);
                if (!header) {
                    header = 1;
                    stream_printf(out, "EFFECTS\n");
                }
                stream_printf(out, "\"%d %s\"\n", data->value, translate(key,
                    LOC(default_locale, key)));
            }
            a = a->next;
        }
        else {
            a = a->nexttype;
        }
    }
}

static void cr_output_curses_compat(FILE *F, const faction * viewer, const void *obj, objtype_t typ) {
    // TODO: eliminate this function
    stream strm;
    fstream_init(&strm, F);
    cr_output_curses(&strm, viewer, obj, typ);
}

static int cr_unit(variant var, char *buffer, const void *userdata)
{
    unit *u = (unit *)var.v;
    sprintf(buffer, "%d", u ? u->no : -1);
    return 0;
}

static int cr_ship(variant var, char *buffer, const void *userdata)
{
    ship *u = (ship *)var.v;
    sprintf(buffer, "%d", u ? u->no : -1);
    return 0;
}

static int cr_building(variant var, char *buffer, const void *userdata)
{
    building *u = (building *)var.v;
    sprintf(buffer, "%d", u ? u->no : -1);
    return 0;
}

static int cr_faction(variant var, char *buffer, const void *userdata)
{
    faction *f = (faction *)var.v;
    sprintf(buffer, "%d", f ? f->no : -1);
    return 0;
}

static int cr_region(variant var, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    region *r = (region *)var.v;
    if (r) {
        plane *pl = rplane(r);
        int nx = r->x, ny = r->y;
        pnormalize(&nx, &ny, pl);
        adjust_coordinates(report, &nx, &ny, pl);
        sprintf(buffer, "%d %d %d", nx, ny, plane_id(pl));
        return 0;
    }
    return -1;
}

static int cr_resource(variant var, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const resource_type *r = (const resource_type *)var.v;
    if (r) {
        const char *key = resourcename(r, 0);
        sprintf(buffer, "\"%s\"",
            translate(key, LOC(report->locale, key)));
        return 0;
    }
    return -1;
}

static int cr_race(variant var, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const struct race *rc = (const race *)var.v;
    const char *key = rc_name_s(rc, NAME_SINGULAR);
    sprintf(buffer, "\"%s\"",
        translate(key, LOC(report->locale, key)));
    return 0;
}

static int cr_alliance(variant var, char *buffer, const void *userdata)
{
    const alliance *al = (const alliance *)var.v;
    unused_arg(userdata);
    if (al != NULL) {
        sprintf(buffer, "%d", al->id);
    }
    return 0;
}

static int cr_skill(variant var, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    skill_t sk = (skill_t)var.i;
    unused_arg(userdata);
    if (sk != NOSKILL)
        sprintf(buffer, "\"%s\"",
        translate(mkname("skill", skillnames[sk]), skillname(sk,
        report->locale)));
    else
        strcpy(buffer, "\"\"");
    return 0;
}

static int cr_order(variant var, char *buffer, const void *userdata)
{
    order *ord = (order *)var.v;
    unused_arg(userdata);
    if (ord != NULL) {
        char cmd[ORDERSIZE];
        char *wp = buffer;
        const char *rp;

        get_command(ord, cmd, sizeof(cmd));

        *wp++ = '\"';
        for (rp = cmd; *rp;) {
            char r = *rp++;
            if (r == '\"' || r == '\\') {
                *wp++ = '\\';
            }
            *wp++ = r;
        }
        *wp++ = '\"';
        *wp++ = 0;
    }
    else
        strcpy(buffer, "\"\"");
    return 0;
}

static int cr_resources(variant var, char *buffer, const void *userdata)
{
    faction *f = (faction *)userdata;
    resource *rlist = (resource *)var.v;
    char *wp = buffer;
    if (rlist != NULL) {
        const char *name = resourcename(rlist->type, rlist->number != 1);
        wp +=
            sprintf(wp, "\"%d %s", rlist->number, translate(name, LOC(f->locale,
            name)));
        for (;;) {
            rlist = rlist->next;
            if (rlist == NULL)
                break;
            name = resourcename(rlist->type, rlist->number != 1);
            wp +=
                sprintf(wp, ", %d %s", rlist->number, translate(name,
                LOC(f->locale, name)));
        }
        strcat(wp, "\"");
    }
    return 0;
}

static int cr_regions(variant var, char *buffer, const void *userdata)
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
        strcat(wp, "\"");
    }
    else {
        strcpy(buffer, "\"\"");
    }
    return 0;
}

static int cr_spell(variant var, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    spell *sp = (spell *)var.v;
    if (sp != NULL)
        sprintf(buffer, "\"%s\"", spell_name(sp, report->locale));
    else
        strcpy(buffer, "\"\"");
    return 0;
}

static int cr_curse(variant var, char *buffer, const void *userdata)
{
    const faction *report = (const faction *)userdata;
    const curse_type *ctype = (const curse_type *)var.v;
    if (ctype != NULL) {
        sprintf(buffer, "\"%s\"", curse_name(ctype, report->locale));
    }
    else
        strcpy(buffer, "\"\"");
    return 0;
}

/*static int msgno; */

#define MTMAXHASH 1021

static struct known_mtype {
    const struct message_type *mtype;
    struct known_mtype *nexthash;
} *mtypehash[MTMAXHASH];

static void report_crtypes(FILE * F, const struct locale *lang)
{
    int i;
    for (i = 0; i != MTMAXHASH; ++i) {
        struct known_mtype *kmt;
        for (kmt = mtypehash[i]; kmt; kmt = kmt->nexthash) {
            const struct nrmessage_type *nrt = nrt_find(lang, kmt->mtype);
            if (nrt) {
                char buffer[DISPLAYSIZE];
                unsigned int hash = kmt->mtype->key;
                assert(hash > 0);
                fprintf(F, "MESSAGETYPE %u\n", hash);
                fputc('\"', F);
                fputs(escape_string(nrt_string(nrt), buffer, sizeof(buffer)), F);
                fputs("\";text\n", F);
                fprintf(F, "\"%s\";section\n", nrt_section(nrt));
            }
        }
        while (mtypehash[i]) {
            kmt = mtypehash[i];
            mtypehash[i] = mtypehash[i]->nexthash;
            free(kmt);
        }
    }
}

static unsigned int messagehash(const struct message *msg)
{
    variant var;
    var.v = (void *)msg;
    return (unsigned int)var.i;
}

/** writes a quoted string to the file
* no trailing space, since this is used to make the creport.
*/
static int fwritestr(FILE * F, const char *str)
{
    int nwrite = 0;
    fputc('\"', F);
    if (str)
        while (*str) {
            int c = (int)(unsigned char)*str++;
            switch (c) {
            case '"':
            case '\\':
                fputc('\\', F);
                fputc(c, F);
                nwrite += 2;
                break;
            case '\n':
                fputc('\\', F);
                fputc('n', F);
                nwrite += 2;
                break;
            default:
                fputc(c, F);
                ++nwrite;
            }
        }
    fputc('\"', F);
    return nwrite + 2;
}

static void render_messages(FILE * F, faction * f, message_list * msgs)
{
    struct mlist *m = msgs->begin;
    while (m) {
        char crbuffer[BUFFERSIZE];  /* gross, wegen spionage-messages :-( */
        bool printed = false;
        const struct message_type *mtype = m->msg->type;
        unsigned int hash = mtype->key;
#ifdef RENDER_CRMESSAGES
        char nrbuffer[1024 * 32];
        nrbuffer[0] = '\0';
        if (nr_render(m->msg, f->locale, nrbuffer, sizeof(nrbuffer), f) > 0) {
            fprintf(F, "MESSAGE %u\n", messagehash(m->msg));
            fprintf(F, "%u;type\n", hash);
            fwritestr(F, nrbuffer);
            fputs(";rendered\n", F);
            printed = true;
        }
#endif
        crbuffer[0] = '\0';
        if (cr_render(m->msg, crbuffer, (const void *)f) == 0) {
            if (crbuffer[0]) {
                if (!printed) {
                    fprintf(F, "MESSAGE %u\n", messagehash(m->msg));
                }
                fputs(crbuffer, F);
            }
        }
        else {
            log_error("could not render cr-message %p: %s\n", m->msg, m->msg->type->name);
        }
        if (printed) {
            unsigned int ihash = hash % MTMAXHASH;
            struct known_mtype *kmt = mtypehash[ihash];
            while (kmt && kmt->mtype != mtype)
                kmt = kmt->nexthash;
            if (kmt == NULL) {
                kmt = (struct known_mtype *)malloc(sizeof(struct known_mtype));
                kmt->nexthash = mtypehash[ihash];
                kmt->mtype = mtype;
                mtypehash[ihash] = kmt;
            }
        }
        m = m->next;
    }
}

static void cr_output_messages(FILE * F, message_list * msgs, faction * f)
{
    if (msgs)
        render_messages(F, f, msgs);
}

/* prints a building */
static void
cr_output_building(FILE * F, building * b, const unit * owner, int fno,
faction * f)
{
    const char *bname, *billusion;

    fprintf(F, "BURG %d\n", b->no);

    report_building(b, &bname, &billusion);
    if (billusion) {
        fprintf(F, "\"%s\";Typ\n", translate(billusion, LOC(f->locale,
            billusion)));
        if (owner && owner->faction == f) {
            fprintf(F, "\"%s\";wahrerTyp\n", translate(bname, LOC(f->locale,
                bname)));
        }
    }
    else {
        fprintf(F, "\"%s\";Typ\n", translate(bname, LOC(f->locale, bname)));
    }
    fprintf(F, "\"%s\";Name\n", b->name);
    if (b->display && b->display[0])
        fprintf(F, "\"%s\";Beschr\n", b->display);
    if (b->size)
        fprintf(F, "%d;Groesse\n", b->size);
    if (owner) {
        fprintf(F, "%d;Besitzer\n", owner->no);
    }
    if (fno >= 0)
        fprintf(F, "%d;Partei\n", fno);
    if (b->besieged)
        fprintf(F, "%d;Belagerer\n", b->besieged);
    cr_output_curses_compat(F, f, b, TYP_BUILDING);
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints a ship */
static void
cr_output_ship(FILE * F, const ship * sh, const unit * u, int fcaptain,
const faction * f, const region * r)
{
    int w = 0;
    assert(sh);
    fprintf(F, "SCHIFF %d\n", sh->no);
    fprintf(F, "\"%s\";Name\n", sh->name);
    if (sh->display && sh->display[0])
        fprintf(F, "\"%s\";Beschr\n", sh->display);
    fprintf(F, "\"%s\";Typ\n", translate(sh->type->_name,
        LOC(f->locale, sh->type->_name)));
    fprintf(F, "%d;Groesse\n", sh->size);
    if (sh->damage) {
        int percent =
            (sh->damage * 100 + DAMAGE_SCALE - 1) / (sh->size * DAMAGE_SCALE);
        fprintf(F, "%d;Schaden\n", percent);
    }
    if (u) {
        fprintf(F, "%d;Kapitaen\n", u->no);
    }
    if (fcaptain >= 0)
        fprintf(F, "%d;Partei\n", fcaptain);

    /* calculate cargo */
    if (u && (u->faction == f || omniscient(f))) {
        int n = 0, p = 0;
        int mweight = shipcapacity(sh);
        getshipweight(sh, &n, &p);

        fprintf(F, "%d;capacity\n", mweight);
        fprintf(F, "%d;cargo\n", n);
        fprintf(F, "%d;speed\n", shipspeed(sh, u));
    }
    /* shore */
    w = NODIRECTION;
    if (!fval(r->terrain, SEA_REGION))
        w = sh->coast;
    if (w != NODIRECTION)
        fprintf(F, "%d;Kueste\n", w);

    cr_output_curses_compat(F, f, sh, TYP_SHIP);
}

static int stream_order(stream *out, const struct order *ord) {
    const char *str;
    char ebuf[1025];
    char obuf[1024];
    write_order(ord, obuf, sizeof(obuf));
    str = escape_string(obuf, ebuf, sizeof(ebuf));
    if (str == ebuf) {
        ebuf[1024] = 0;
    }
    return stream_printf(out, "\"%s\"\n", str);
}

static void cr_output_spells(stream *out, const unit * u, int maxlevel)
{
    spellbook * book = unit_get_spellbook(u);

    if (book) {
        const faction * f = u->faction;
        quicklist *ql;
        int qi, header = 0;

        for (ql = book->spells, qi = 0; ql; ql_advance(&ql, &qi, 1)) {
            spellbook_entry * sbe = (spellbook_entry *)ql_get(ql, qi);
            if (sbe->level <= maxlevel) {
                spell * sp = sbe->sp;
                const char *name = translate(mkname("spell", sp->sname), spell_name(sp, f->locale));
                if (!header) {
                    stream_printf(out, "SPRUECHE\n");
                    header = 1;
                }
                stream_printf(out, "\"%s\"\n", name);
            }
        }
    }
}

/** prints all that belongs to a unit
* @param f observers faction
* @param u unit to report
*/
void cr_output_unit(stream *out, const region * r, const faction * f,
    const unit * u, seen_mode mode)
{
    /* Race attributes are always plural and item attributes always
     * singular */
    const char *str;
    const item_type *lasttype;
    int pr;
    item *itm, *show = NULL;
    building *b;
    const char *pzTmp;
    skill *sv;
    item result[MAX_INVENTORY];
    const faction *sf;
    const char *prefix;

    assert(u && u->number);
    assert(u->region == r); // TODO: if this holds true, then why did we pass in r?
    if (fval(u_race(u), RCF_INVISIBLE))
        return;

    stream_printf(out, "EINHEIT %d\n", u->no);
    stream_printf(out, "\"%s\";Name\n", unit_getname(u));
    str = u_description(u, f->locale);
    if (str) {
        stream_printf(out, "\"%s\";Beschr\n", str);
    }
    /* print faction information */
    sf = visible_faction(f, u);
    prefix = raceprefix(u);
    if (u->faction == f || omniscient(f)) {
        const attrib *a_otherfaction = a_find(u->attribs, &at_otherfaction);
        const faction *otherfaction =
            a_otherfaction ? get_otherfaction(a_otherfaction) : NULL;
        /* my own faction, full info */
        const attrib *a = NULL;
        unit *mage;

        if (fval(u, UFL_GROUP))
            a = a_find(u->attribs, &at_group);
        if (a != NULL) {
            const group *g = (const group *)a->data.v;
            stream_printf(out, "%d;gruppe\n", g->gid);
        }
        stream_printf(out, "%d;Partei\n", u->faction->no);
        if (sf != u->faction)
            stream_printf(out, "%d;Verkleidung\n", sf->no);
        if (fval(u, UFL_ANON_FACTION))
            stream_printf(out, "%d;Parteitarnung\n", i2b(fval(u, UFL_ANON_FACTION)));
        if (otherfaction && otherfaction != u->faction) {
            stream_printf(out, "%d;Anderepartei\n", otherfaction->no);
        }
        mage = get_familiar_mage(u);
        if (mage) {
            stream_printf(out, "%u;familiarmage\n", mage->no);
        }
    }
    else {
        if (fval(u, UFL_ANON_FACTION)) {
            /* faction info is hidden */
            stream_printf(out, "%d;Parteitarnung\n", i2b(fval(u, UFL_ANON_FACTION)));
        }
        else {
            const attrib *a_otherfaction = a_find(u->attribs, &at_otherfaction);
            const faction *otherfaction =
                a_otherfaction ? get_otherfaction(a_otherfaction) : NULL;
            /* other unit. show visible faction, not u->faction */
            stream_printf(out, "%d;Partei\n", sf->no);
            if (sf == f) {
                stream_printf(out, "1;Verraeter\n");
            }
            if (otherfaction && otherfaction != u->faction) {
                if (alliedunit(u, f, HELP_FSTEALTH)) {
                    stream_printf(out, "%d;Anderepartei\n", otherfaction->no);
                }
            }
        }
    }
    if (prefix) {
        prefix = mkname("prefix", prefix);
        stream_printf(out, "\"%s\";typprefix\n", translate(prefix, LOC(f->locale,
            prefix)));
    }
    stream_printf(out, "%d;Anzahl\n", u->number);

    pzTmp = get_racename(u->attribs);
    if (pzTmp) {
        stream_printf(out, "\"%s\";Typ\n", pzTmp);
        if (u->faction == f && fval(u_race(u), RCF_SHAPESHIFTANY)) {
            const char *zRace = rc_name_s(u_race(u), NAME_PLURAL);
            stream_printf(out, "\"%s\";wahrerTyp\n",
                translate(zRace, LOC(f->locale, zRace)));
        }
    }
    else {
        const race *irace = u_irace(u);
        const char *zRace = rc_name_s(irace, NAME_PLURAL);
        stream_printf(out, "\"%s\";Typ\n",
            translate(zRace, LOC(f->locale, zRace)));
        if (u->faction == f && irace != u_race(u)) {
            assert(skill_enabled(SK_STEALTH)
                || !"we're resetting this on load, so.. ircase should never be used");
            zRace = rc_name_s(u_race(u), NAME_PLURAL);
            stream_printf(out, "\"%s\";wahrerTyp\n",
                translate(zRace, LOC(f->locale, zRace)));
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
    if (is_guard(u, GUARD_ALL) != 0) {
        stream_printf(out, "%d;bewacht\n", 1);
    }
    if ((b = usiege(u)) != NULL) {
        stream_printf(out, "%d;belagert\n", b->no);
    }
    /* additional information for own units */
    if (u->faction == f || omniscient(f)) {
        order *ord;
        const char *xc;
        const char *c;
        int i;
        sc_mage *mage;

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
        if (is_mage(u)) {
            stream_printf(out, "%d;Aura\n", get_spellpoints(u));
            stream_printf(out, "%d;Auramax\n", max_spellpoints(u->region, u));
        }
        /* default commands */
        stream_printf(out, "COMMANDS\n");
        for (ord = u->old_orders; ord; ord = ord->next) {
            /* this new order will replace the old defaults */
            if (is_persistent(ord)) {
                stream_order(out, ord);
            }
        }
        for (ord = u->orders; ord; ord = ord->next) {
            keyword_t kwd = getkeyword(ord);
            if (u->old_orders && is_repeated(kwd))
                continue;               /* unit has defaults */
            if (is_persistent(ord)) {
                stream_order(out, ord);
            }
        }

        /* talents */
        pr = 0;
        for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
            if (sv->level > 0) {
                skill_t sk = sv->id;
                int esk = effskill(u, sk, 0);
                if (!pr) {
                    pr = 1;
                    stream_printf(out, "TALENTE\n");
                }
                stream_printf(out, "%d %d;%s\n", u->number * level_days(sv->level), esk,
                    translate(mkname("skill", skillnames[sk]), skillname(sk,
                    f->locale)));
            }
        }

        /* spells that this unit can cast */
        mage = get_mage(u);
        if (mage) {
            int i, maxlevel = effskill(u, SK_MAGIC, 0);
            cr_output_spells(out, u, maxlevel);

            for (i = 0; i != MAXCOMBATSPELLS; ++i) {
                const spell *sp = mage->combatspells[i].sp;
                if (sp) {
                    const char *name =
                        translate(mkname("spell", sp->sname), spell_name(sp,
                        f->locale));
                    stream_printf(out, "KAMPFZAUBER %d\n", i);
                    stream_printf(out, "\"%s\";name\n", name);
                    stream_printf(out, "%d;level\n", mage->combatspells[i].level);
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
    lasttype = NULL;
    for (itm = show; itm; itm = itm->next) {
        const char *ic;
        int in;
        assert(itm->type != lasttype
            || !"error: list contains two objects of the same item");
        report_item(u, itm, f, NULL, &ic, &in, true);
        if (in == 0)
            continue;
        if (!pr) {
            pr = 1;
            stream_printf(out, "GEGENSTAENDE\n");
        }
        stream_printf(out, "%d;%s\n", in, translate(ic, LOC(f->locale, ic)));
    }

    cr_output_curses(out, f, u, TYP_UNIT);
}

static void cr_output_unit_compat(FILE * F, const region * r, const faction * f,
    const unit * u, int mode)
{
    // TODO: eliminate this function
    stream strm;
    fstream_init(&strm, F);
    cr_output_unit(&strm, r, f, u, mode);
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints allies */
static void show_allies_cr(FILE * F, const faction * f, const ally * sf)
{
    for (; sf; sf = sf->next)
        if (sf->faction) {
            int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
            if (mode != 0 && sf->status > 0) {
                fprintf(F, "ALLIANZ %d\n", sf->faction->no);
                fprintf(F, "\"%s\";Parteiname\n", sf->faction->name);
                fprintf(F, "%d;Status\n", sf->status & HELP_ALL);
            }
        }
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
static void cr_find_address(FILE * F, const faction * uf, quicklist * addresses)
{
    int i = 0;
    quicklist *flist = addresses;
    while (flist) {
        const faction *f = (const faction *)ql_get(flist, i);
        if (uf != f) {
            fprintf(F, "PARTEI %d\n", f->no);
            fprintf(F, "\"%s\";Parteiname\n", f->name);
            if (f->email)
                fprintf(F, "\"%s\";email\n", f->email);
            if (f->banner)
                fprintf(F, "\"%s\";banner\n", f->banner);
            fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
            if (f->alliance && f->alliance == uf->alliance) {
                fprintf(F, "%d;alliance\n", f->alliance->id);
            }
        }
        ql_advance(&flist, &i, 1);
    }
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

static void cr_reportspell(FILE * F, spell * sp, int level, const struct locale *lang)
{
    int k;
    const char *name =
        translate(mkname("spell", sp->sname), spell_name(sp, lang));

    fprintf(F, "ZAUBER %d\n", hashstring(sp->sname));
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
            const char *name = resourcename(rtype, 0);
            fprintf(F, "%d %d;%s\n", itemanz, costtyp == SPC_LEVEL
                || costtyp == SPC_LINEAR, translate(name, LOC(lang, name)));
        }
    }
}

static char *cr_output_resource(char *buf, const char *name,
    const struct locale *loc, int amount, int level)
{
    buf += sprintf(buf, "RESOURCE %u\n", hashstring(name));
    buf += sprintf(buf, "\"%s\";type\n", translate(name, LOC(loc, name)));
    if (amount >= 0) {
        if (level >= 0)
            buf += sprintf(buf, "%d;skill\n", level);
        buf += sprintf(buf, "%d;number\n", amount);
    }
    return buf;
}

static void
cr_borders(const region * r, const faction * f, seen_mode mode, FILE * F)
{
    direction_t d;
    int g = 0;
    for (d = 0; d != MAXDIRECTIONS; d++) {        /* Nachbarregionen, die gesehen werden, ermitteln */
        const region *r2 = rconnect(r, d);
        const connection *b;
        if (!r2)
            continue;
        if (mode == seen_neighbour) {
            if (r2->seen.mode <= seen_neighbour)
                continue;
        }
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
                const char *bname = border_name(b, r, f, GF_PURE);
                bname = mkname("border", bname);
                fprintf(F, "GRENZE %d\n", ++g);
                fprintf(F, "\"%s\";typ\n", LOC(default_locale, bname));
                fprintf(F, "%d;richtung\n", d);
                if (!b->type->transparent(b, f))
                    fputs("1;opaque\n", F);
                /* hack: */
                if (b->type == &bt_road && r->terrain->max_road) {
                    int p = rroad(r, d) * 100 / r->terrain->max_road;
                    fprintf(F, "%d;prozent\n", p);
                }
            }
            b = b->next;
        }
    }
}

static void
cr_output_resources(FILE * F, report_context * ctx, region *r, bool see_unit)
{
    char cbuf[BUFFERSIZE], *pos = cbuf;
    faction *f = ctx->f;
    resource_report result[MAX_RAWMATERIALS];
    int n, size = report_resources(r, result, MAX_RAWMATERIALS, f, see_unit);

#ifdef RESOURCECOMPAT
    int trees = rtrees(r, 2);
    int saplings = rtrees(r, 1);

    if (trees > 0)
        fprintf(F, "%d;Baeume\n", trees);
    if (saplings > 0)
        fprintf(F, "%d;Schoesslinge\n", saplings);
    if (fval(r, RF_MALLORN) && (trees > 0 || saplings > 0))
        fprintf(F, "1;Mallorn\n");
    for (n = 0; n < size; ++n) {
        if (result[n].level >= 0 && result[n].number >= 0) {
            fprintf(F, "%d;%s\n", result[n].number, crtag(result[n].name));
        }
    }
#endif

    for (n = 0; n < size; ++n) {
        if (result[n].number >= 0) {
            pos =
                cr_output_resource(pos, result[n].name, f->locale, result[n].number,
                result[n].level);
        }
    }
    if (pos != cbuf)
        fputs(cbuf, F);
}

static void
cr_region_header(FILE * F, int plid, int nx, int ny, int uid)
{
    if (plid == 0) {
        fprintf(F, "REGION %d %d\n", nx, ny);
    }
    else {
        fprintf(F, "REGION %d %d %d\n", nx, ny, plid);
    }
    if (uid)
        fprintf(F, "%d;id\n", uid);
}

typedef struct travel_data {
    const faction *f;
    FILE *file;
    int n;
} travel_data;

static void cb_cr_travelthru_ship(region *r, unit *u, void *cbdata) {
    travel_data *data = (travel_data *)cbdata;
    const faction *f = data->f;
    FILE *F = data->file;

    if (u->ship && travelthru_cansee(r, f, u)) {
        if (data->n++ == 0) {
            fprintf(F, "DURCHSCHIFFUNG\n");
        }
        fprintf(F, "\"%s\"\n", shipname(u->ship));
    }
}

static void cb_cr_travelthru_unit(region *r, unit *u, void *cbdata) {
    travel_data *data = (travel_data *)cbdata;
    const faction *f = data->f;
    FILE *F = data->file;

    if (!u->ship && travelthru_cansee(r, f, u)) {
        if (data->n++ == 0) {
            fprintf(F, "DURCHREISE\n");
        }
        fprintf(F, "\"%s\"\n", unitname(u));
    }
}

static void cr_output_travelthru(FILE *F, region *r, const faction *f) {
    /* describe both passed and inhabited regions */
    travel_data cbdata = { 0 };
    cbdata.f = f;
    cbdata.file = F;
    cbdata.n = 0;
    travelthru_map(r, cb_cr_travelthru_ship, &cbdata);
    cbdata.n = 0;
    travelthru_map(r, cb_cr_travelthru_unit, &cbdata);
}

static void cr_output_region(FILE * F, report_context * ctx, region * r)
{
    faction *f = ctx->f;
    plane *pl = rplane(r);
    int plid = plane_id(pl), nx, ny;
    const char *tname;
    int oc[4][2], o = 0;
    int uid = r->uid;

#ifdef OCEAN_NEIGHBORS_GET_NO_ID
    if (r->seen.mode <= seen_neighbour && !r->land) {
        uid = 0;
    }
#endif

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
        cr_region_header(F, plid, oc[o][0], oc[o][1], uid);
        fputs("\"wrap\";visibility\n", F);
    }

    cr_region_header(F, plid, nx, ny, uid);

    if (r->land) {
        const char *str = rname(r, f->locale);
        if (str && str[0]) {
            fprintf(F, "\"%s\";Name\n", str);
        }
    }
    tname = terrain_name(r);

    fprintf(F, "\"%s\";Terrain\n", translate(tname, LOC(f->locale, tname)));
    if (r->seen.mode != seen_unit)
        fprintf(F, "\"%s\";visibility\n", visibility[r->seen.mode]);
    if (r->seen.mode == seen_neighbour) {
        cr_borders(r, f, r->seen.mode, F);
    }
    else {
        building *b;
        ship *sh;
        unit *u;
        int stealthmod = stealth_modifier(r->seen.mode);

        if (r->display && r->display[0])
            fprintf(F, "\"%s\";Beschr\n", r->display);
        if (fval(r->terrain, LAND_REGION)) {
            assert(r->land);
            fprintf(F, "%d;Bauern\n", rpeasants(r));
            if (fval(r, RF_ORCIFIED)) {
                fprintf(F, "1;Verorkt\n");
            }
            fprintf(F, "%d;Pferde\n", rhorses(r));

            if (r->seen.mode >= seen_unit) {
                if (rule_region_owners()) {
                    faction *owner = region_get_owner(r);
                    if (owner) {
                        fprintf(F, "%d;owner\n", owner->no);
                    }
                }
                fprintf(F, "%d;Silber\n", rmoney(r));
                if (skill_enabled(SK_ENTERTAINMENT)) {
                    fprintf(F, "%d;Unterh\n", entertainmoney(r));
                }
                if (is_cursed(r->attribs, C_RIOT, 0)) {
                    fputs("0;Rekruten\n", F);
                }
                else {
                    fprintf(F, "%d;Rekruten\n", rpeasants(r) / RECRUITFRACTION);
                }
                if (production(r)) {
                    int p_wage = wage(r, NULL, NULL, turn + 1);
                    fprintf(F, "%d;Lohn\n", p_wage);
                    if (is_mourning(r, turn + 1)) {
                        fputs("1;mourning\n", F);
                    }
                }
                if (r->land && r->land->ownership) {
                    fprintf(F, "%d;morale\n", region_get_morale(r));
                }
            }

            /* this writes both some tags (RESOURCECOMPAT) and a block.
             * must not write any blocks before it */
            cr_output_resources(F, ctx, r, r->seen.mode >= seen_unit);

            if (r->seen.mode >= seen_unit) {
                /* trade */
                if (markets_module() && r->land) {
                    const item_type *lux = r_luxury(r);
                    const item_type *herb = r->land->herbtype;
                    if (lux || herb) {
                        fputs("PREISE\n", F);
                        if (lux) {
                            const char *ch = resourcename(lux->rtype, 0);
                            fprintf(F, "%d;%s\n", 1, translate(ch,
                                LOC(f->locale, ch)));
                        }
                        if (herb) {
                            const char *ch = resourcename(herb->rtype, 0);
                            fprintf(F, "%d;%s\n", 1, translate(ch,
                                LOC(f->locale, ch)));
                        }
                    }
                }
                else if (rpeasants(r) / TRADE_FRACTION > 0) {
                    struct demand *dmd = r->land->demands;
                    fputs("PREISE\n", F);
                    while (dmd) {
                        const char *ch = resourcename(dmd->type->itype->rtype, 0);
                        fprintf(F, "%d;%s\n", (dmd->value
                            ? dmd->value * dmd->type->price
                            : -dmd->type->price),
                            translate(ch, LOC(f->locale, ch)));
                        dmd = dmd->next;
                    }
                }
            }
        }
        if (r->land) {
            print_items(F, r->land->items, f->locale);
        }
        cr_output_curses_compat(F, f, r, TYP_REGION);
        cr_borders(r, f, r->seen.mode, F);
        if (r->seen.mode == seen_unit && is_astral(r)
            && !is_cursed(r->attribs, C_ASTRALBLOCK, 0)) {
            /* Sonderbehandlung Teleport-Ebene */
            region_list *rl = astralregions(r, inhabitable);

            if (rl) {
                region_list *rl2 = rl;
                while (rl2) {
                    region *r = rl2->data;
                    int nx = r->x, ny = r->y;
                    plane *plx = rplane(r);

                    pnormalize(&nx, &ny, plx);
                    adjust_coordinates(f, &nx, &ny, plx);
                    fprintf(F, "SCHEMEN %d %d\n", nx, ny);
                    fprintf(F, "\"%s\";Name\n", rname(r, f->locale));
                    rl2 = rl2->next;
                }
                free_regionlist(rl);
            }
        }

        cr_output_travelthru(F, r, f);
        if (r->seen.mode == seen_unit || r->seen.mode == seen_travel) {
            message_list *mlist = r_getmessages(r, f);
            cr_output_messages(F, r->msgs, f);
            if (mlist) {
                cr_output_messages(F, mlist, f);
            }
        }
        /* buildings */
        for (b = rbuildings(r); b; b = b->next) {
            int fno = -1;
            u = building_owner(b);
            if (u && !fval(u, UFL_ANON_FACTION)) {
                const faction *sf = visible_faction(f, u);
                fno = sf->no;
            }
            cr_output_building(F, b, u, fno, f);
        }

        /* ships */
        for (sh = r->ships; sh; sh = sh->next) {
            int fno = -1;
            u = ship_owner(sh);
            if (u && !fval(u, UFL_ANON_FACTION)) {
                const faction *sf = visible_faction(f, u);
                fno = sf->no;
            }

            cr_output_ship(F, sh, u, fno, f, r);
        }

        /* visible units */
        for (u = r->units; u; u = u->next) {

            if (u->building || u->ship || (stealthmod > INT_MIN
                && cansee(f, r, u, stealthmod))) {
                cr_output_unit_compat(F, r, f, u, r->seen.mode);
            }
        }
    }
}

/* main function of the creport. creates the header and traverses all regions */
static int
report_computer(const char *filename, report_context * ctx, const char *charset)
{
    static int era = -1;
    int i;
    faction *f = ctx->f;
    const char *prefix;
    region *r;
    const char *mailto = LOC(f->locale, "mailto");
    const attrib *a;
    FILE *F = fopen(filename, "w");

    if (era < 0) {
        era = config_get_int("world.era", 1);
    }
    if (F == NULL) {
        perror(filename);
        return -1;
    }
    else if (_strcmpl(charset, "utf-8") == 0 || _strcmpl(charset, "utf8") == 0) {
        const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf, 0 };
        fwrite(utf8_bom, 1, 3, F);
    }

    /* must call this to get all the neighbour regions */
    /* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
    /* initialisations, header and lists */

    fprintf(F, "VERSION %d\n", C_REPORT_VERSION);
    fprintf(F, "\"%s\";charset\n", charset);
    fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
    fprintf(F, "%d;noskillpoints\n", 1);
    fprintf(F, "%lld;date\n", (long long)ctx->report_time);
    fprintf(F, "\"%s\";Spiel\n", game_name());
    fprintf(F, "\"%s\";Konfiguration\n", "Standard");
    fprintf(F, "\"%s\";Koordinaten\n", "Hex");
    fprintf(F, "%d;max_units\n", rule_faction_limit());
    fprintf(F, "%d;Basis\n", 36);
    fprintf(F, "%d;Runde\n", turn);
    fprintf(F, "%d;Zeitalter\n", era);
    fprintf(F, "\"%s\";Build\n", ERESSEA_VERSION);
    if (mailto != NULL) {
        fprintf(F, "\"%s\";mailto\n", mailto);
        fprintf(F, "\"%s\";mailcmd\n", LOC(f->locale, "mailcmd"));
    }

    show_alliances_cr(F, f);

    fprintf(F, "PARTEI %d\n", f->no);
    fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
    if (f_get_alliance(f)) {
        fprintf(F, "%d;alliance\n", f->alliance->id);
        fprintf(F, "%d;joined\n", f->alliance_joindate);
    }
    fprintf(F, "%d;age\n", f->age);
    fprintf(F, "%d;Optionen\n", f->options);
    if (f->options & want(O_SCORE) && f->age > DISPLAYSCORE) {
        char score[32];
        write_score(score, sizeof(score), f->score);
        fprintf(F, "%s;Punkte\n", score);
        write_score(score, sizeof(score), average_score_of_age(f->age, f->age / 24 + 1));
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
    fprintf(F, "%d;Anzahl Personen\n", count_all(f));
    fprintf(F, "\"%s\";Magiegebiet\n", magic_school[f->magiegebiet]);

    if (f->race == get_race(RC_HUMAN)) {
        fprintf(F, "%d;Anzahl Immigranten\n", count_migrants(f));
        fprintf(F, "%d;Max. Immigranten\n", count_maxmigrants(f));
    }

    i = countheroes(f);
    if (i > 0)
        fprintf(F, "%d;heroes\n", i);
    i = maxheroes(f);
    if (i > 0)
        fprintf(F, "%d;max_heroes\n", i);

    if (f->age > 1 && f->lastorders != turn) {
        fprintf(F, "%d;nmr\n", turn - f->lastorders);
    }

    fprintf(F, "\"%s\";Parteiname\n", f->name);
    fprintf(F, "\"%s\";email\n", f->email);
    if (f->banner)
        fprintf(F, "\"%s\";banner\n", f->banner);
    print_items(F, f->items, f->locale);
    fputs("OPTIONEN\n", F);
    for (i = 0; i != MAXOPTIONS; ++i) {
        int flag = want(i);
        if (options[i]) {
            fprintf(F, "%d;%s\n", (f->options & flag) ? 1 : 0, options[i]);
        }
        else if (f->options & flag) {
            f->options &= (~flag);
        }
    }
    show_allies_cr(F, f, f->allies);
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
            show_allies_cr(F, f, g->allies);
        }
    }

    cr_output_messages(F, f->msgs, f);
    {
        struct bmsg *bm;
        for (bm = f->battles; bm; bm = bm->next) {
            plane *pl = rplane(bm->r);
            int plid = plane_id(pl);
            region *r = bm->r;
            int nx = r->x, ny = r->y;

            pnormalize(&nx, &ny, pl);
            adjust_coordinates(f, &nx, &ny, pl);
            if (!plid)
                fprintf(F, "BATTLE %d %d\n", nx, ny);
            else {
                fprintf(F, "BATTLE %d %d %d\n", nx, ny, plid);
            }
            cr_output_messages(F, bm->msgs, f);
        }
    }

    cr_find_address(F, f, ctx->addresses);
    a = a_find(f->attribs, &at_reportspell);
    while (a && a->type == &at_reportspell) {
        spellbook_entry *sbe = (spellbook_entry *)a->data.v;
        cr_reportspell(F, sbe->sp, sbe->level, f->locale);
        a = a->next;
    }
    for (a = a_find(f->attribs, &at_showitem); a && a->type == &at_showitem;
        a = a->next) {
        const potion_type *ptype =
            resource2potion(((const item_type *)a->data.v)->rtype);
        const char *ch;
        const char *description = NULL;

        if (ptype == NULL)
            continue;
        ch = resourcename(ptype->itype->rtype, 0);
        fprintf(F, "TRANK %d\n", hashstring(ch));
        fprintf(F, "\"%s\";Name\n", translate(ch, LOC(f->locale, ch)));
        fprintf(F, "%d;Stufe\n", ptype->level);

        if (description == NULL) {
            const char *pname = resourcename(ptype->itype->rtype, 0);
            const char *potiontext = mkname("potion", pname);
            description = LOC(f->locale, potiontext);
        }

        fprintf(F, "\"%s\";Beschr\n", description);
        if (ptype->itype->construction) {
            requirement *m = ptype->itype->construction->materials;

            fprintf(F, "ZUTATEN\n");

            while (m->number) {
                ch = resourcename(m->rtype, 0);
                fprintf(F, "\"%s\"\n", translate(ch, LOC(f->locale, ch)));
                m++;
            }
        }
    }

    /* traverse all regions */
    for (r = ctx->first; r != ctx->last; r = r->next) {
        cr_output_region(F, ctx, r);
    }
    report_crtypes(F, f->locale);
    write_translations(F);
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
    tsf_register("report", &cr_ignore);
    tsf_register("string", &cr_string);
    tsf_register("order", &cr_order);
    tsf_register("spell", &cr_spell);
    tsf_register("curse", &cr_curse);
    tsf_register("int", &cr_int);
    tsf_register("unit", &cr_unit);
    tsf_register("region", &cr_region);
    tsf_register("faction", &cr_faction);
    tsf_register("ship", &cr_ship);
    tsf_register("building", &cr_building);
    tsf_register("skill", &cr_skill);
    tsf_register("resource", &cr_resource);
    tsf_register("race", &cr_race);
    tsf_register("direction", &cr_int);
    tsf_register("alliance", &cr_alliance);
    tsf_register("resources", &cr_resources);
    tsf_register("items", &cr_resources);
    tsf_register("regions", &cr_regions);

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
