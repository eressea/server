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
#include "save.h"

#include "../buildno.h"

#include "alchemy.h"
#include "alliance.h"
#include "ally.h"
#include "connection.h"
#include "building.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "messages.h"
#include "move.h"
#include "objtypes.h"
#include "order.h"
#include "pathfinder.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "resources.h"
#include "ship.h"
#include "skill.h"
#include "spell.h"
#include "spellbook.h"
#include "terrain.h"
#include "terrainid.h"          /* only for conversion code */
#include "unit.h"
#include "lighthouse.h"
#include "version.h"

/* attributes includes */
#include <attributes/key.h>
#include <triggers/timeout.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/filereader.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <quicklist.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/umlaut.h>
#include <util/unicode.h>

#include <stream.h>
#include <filestream.h>
#include <storage.h>
#include <binarystore.h>

/* libc includes */
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#define xisdigit(c)     (((c) >= '0' && (c) <= '9') || (c) == '-')

#define ESCAPE_FIX
#define MAXORDERS 256
#define MAXPERSISTENT 128

/* exported symbols symbols */
int firstx = 0, firsty = 0;
int enc_gamedata = ENCODING_UTF8;

/* local symbols */
static region *current_region;

char *rns(FILE * f, char *c, size_t size)
{
    char *s = c;
    do {
        *s = (char)getc(f);
    } while (*s != '"');

    for (;;) {
        *s = (char)getc(f);
        if (*s == '"')
            break;
        if (s < c + size)
            ++s;
    }
    *s = 0;
    return c;
}


static unit *unitorders(FILE * F, int enc, struct faction *f)
{
    int i;
    unit *u;

    if (!f)
        return NULL;

    i = getid();
    u = findunitg(i, NULL);

    if (u && u_race(u) == get_race(RC_SPELL))
        return NULL;
    if (u && u->faction == f) {
        order **ordp;

        if (!fval(u, UFL_ORDERS)) {
            /* alle wiederholbaren, langen befehle werden gesichert: */
            fset(u, UFL_ORDERS);
            u->old_orders = u->orders;
            ordp = &u->old_orders;
            while (*ordp) {
                order *ord = *ordp;
                keyword_t kwd = getkeyword(ord);
                if (!is_repeated(kwd)) {
                    *ordp = ord->next;
                    ord->next = NULL;
                    free_order(ord);
                }
                else {
                    ordp = &ord->next;
                }
            }
        }
        else {
            free_orders(&u->orders);
        }
        u->orders = 0;

        ordp = &u->orders;

        for (;;) {
            const char *s;
            /* Erst wenn wir sicher sind, dass kein Befehl
             * eingegeben wurde, checken wir, ob nun eine neue
             * Einheit oder ein neuer Spieler drankommt */

            s = getbuf(F, enc);
            if (s == NULL)
                break;

            if (s[0]) {
                if (s[0] != '@') {
                    char token[128];
                    const char *stok = s;
                    stok = parse_token(&stok, token, sizeof(token));

                    if (stok) {
                        bool quit = false;
                        param_t param = findparam(stok, u->faction->locale);
                        switch (param) {
                        case P_UNIT:
                        case P_REGION:
                            quit = true;
                            break;
                        case P_FACTION:
                        case P_NEXT:
                        case P_GAMENAME:
                            /* these terminate the orders, so we apply extra checking */
                            if (strlen(stok) >= 3) {
                                quit = true;
                                break;
                            }
                            else {
                                quit = false;
                            }
                            break;
                        default:
                            break;
                        }
                        if (quit) {
                            break;
                        }
                    }
                }
                /* Nun wird der Befehl erzeut und eingehängt */
                *ordp = parse_order(s, u->faction->locale);
                if (*ordp) {
                    ordp = &(*ordp)->next;
                }
            }
        }

    }
    else {
        /* cmistake(?, buf, 160, MSG_EVENT); */
        return NULL;
    }
    return u;
}

static faction *factionorders(void)
{
    faction *f = NULL;
    int fid = getid();

    f = findfaction(fid);

    if (f != NULL && !fval(f, FFL_NPC)) {
        char token[128];
        const char *pass = gettoken(token, sizeof(token));

        if (!checkpasswd(f, (const char *)pass)) {
            log_debug("Invalid password for faction %s\n", itoa36(fid));
            ADDMSG(&f->msgs, msg_message("wrongpasswd", "faction password",
                f->no, pass));
            return 0;
        }
        /* Die Partei hat sich zumindest gemeldet, so dass sie noch
         * nicht als untätig gilt */

        /* TODO: +1 ist ein Workaround, weil cturn erst in process_orders
         * incrementiert wird. */
        f->lastorders = global.data_turn + 1;

    }
    else {
        log_debug("orders for invalid faction %s\n", itoa36(fid));
    }
    return f;
}

int readorders(const char *filename)
{
    FILE *F = NULL;
    const char *b;
    int nfactions = 0;
    struct faction *f = NULL;

    F = fopen(filename, "rb");
    if (!F) {
        perror(filename);
        return -1;
    }
    log_info("reading orders from %s", filename);

    /* TODO: recognize UTF8 BOM */
    b = getbuf(F, enc_gamedata);

    /* Auffinden der ersten Partei, und danach abarbeiten bis zur letzten
     * Partei */

    while (b) {
        char token[128];
        const struct locale *lang = f ? f->locale : default_locale;
        param_t p;
        const char *s;
        init_tokens_str(b);
        s = gettoken(token, sizeof(token));
        p = (s && s[0] != '@') ? findparam(s, lang) : NOPARAM;
        switch (p) {
        case P_GAMENAME:
        case P_FACTION:
            f = factionorders();
            if (f) {
                ++nfactions;
            }

            b = getbuf(F, enc_gamedata);
            break;

            /* in factionorders wird nur eine zeile gelesen:
             * diejenige mit dem passwort. Die befehle der units
             * werden geloescht, und die Partei wird als aktiv
             * vermerkt. */

        case P_UNIT:
            if (!f || !unitorders(F, enc_gamedata, f))
                do {
                    b = getbuf(F, enc_gamedata);
                    if (!b) {
                        break;
                    }
                    init_tokens_str(b);
                    s = gettoken(token, sizeof(token));
                    p = (s && s[0] != '@') ? findparam(s, lang) : NOPARAM;
                } while ((p != P_UNIT || !f) && p != P_FACTION && p != P_NEXT
                    && p != P_GAMENAME);
                break;

                /* Falls in unitorders() abgebrochen wird, steht dort entweder eine neue
                 * Partei, eine neue Einheit oder das File-Ende. Das switch() wird erneut
                 * durchlaufen, und die entsprechende Funktion aufgerufen. Man darf buf
                 * auf alle Fälle nicht überschreiben! Bei allen anderen Einträgen hier
                 * muss buf erneut gefüllt werden, da die betreffende Information in nur
                 * einer Zeile steht, und nun die nächste gelesen werden muss. */

        case P_NEXT:
            f = NULL;
            b = getbuf(F, enc_gamedata);
            break;

        default:
            b = getbuf(F, enc_gamedata);
            break;
        }
    }

    fclose(F);
    log_info("done reading orders for %d factions", nfactions);
    return 0;
}

/* ------------------------------------------------------------- */

/* #define INNER_WORLD  */
/* fürs debuggen nur den inneren Teil der Welt laden */
/* -9;-27;-1;-19;Sumpfloch */
int inner_world(region * r)
{
    static int xy[2] = { 18, -45 };
    static int size[2] = { 27, 27 };

    if (r->x >= xy[0] && r->x < xy[0] + size[0] && r->y >= xy[1]
        && r->y < xy[1] + size[1])
        return 2;
    if (r->x >= xy[0] - 9 && r->x < xy[0] + size[0] + 9 && r->y >= xy[1] - 9
        && r->y < xy[1] + size[1] + 9)
        return 1;
    return 0;
}

int maxregions = -1;
int loadplane = 0;

enum {
    U_MAN,
    U_UNDEAD,
    U_ILLUSION,
    U_FIREDRAGON,
    U_DRAGON,
    U_WYRM,
    U_SPELL,
    U_TAVERNE,
    U_MONSTER,
    U_BIRTHDAYDRAGON,
    U_TREEMAN,
    MAXTYPES
};

race_t typus2race(unsigned char typus)
{
    if (typus > 0 && typus <= 11)
        return (race_t)(typus - 1);
    return NORACE;
}

void create_backup(char *file)
{
#ifdef HAVE_LINK
    char bfile[MAX_PATH];
    int c = 1;

    if (access(file, R_OK) == 0)
        return;
    do {
        sprintf(bfile, "%s.backup%d", file, c);
        c++;
    } while (access(bfile, R_OK) == 0);
    link(file, bfile);
#endif
}

void read_items(struct storage *store, item ** ilist)
{
    for (;;) {
        char ibuf[32];
        const item_type *itype;
        int i;
        READ_STR(store, ibuf, sizeof(ibuf));
        if (!strcmp("end", ibuf)) {
            break;
        }
        itype = it_find(ibuf);
        READ_INT(store, &i);
        if (i <= 0) {
            log_error("data contains an entry with %d %s\n", i, resourcename(itype->rtype, NMF_PLURAL));
        }
        else {
            if (itype && itype->rtype) {
                i_change(ilist, itype, i);
            }
            else {
                log_error("data contains unknown item type %s.\n", ibuf);
            }
            assert(itype && itype->rtype);
        }
    }
}

static void read_alliances(struct storage *store)
{
    char pbuf[8];
    int id, terminator = 0;
    if (global.data_version < ALLIANCELEADER_VERSION) {
        terminator = atoi36("end");
        READ_STR(store, pbuf, sizeof(pbuf));
        id = atoi36(pbuf);
    }
    else {
        READ_INT(store, &id);
    }
    while (id != terminator) {
        char aname[128];
        alliance *al;
        READ_STR(store, aname, sizeof(aname));
        al = makealliance(id, aname);
        if (global.data_version >= OWNER_2_VERSION) {
            READ_INT(store, &al->flags);
        }
        if (global.data_version >= ALLIANCELEADER_VERSION) {
            read_reference(&al->_leader, store, read_faction_reference,
                resolve_faction);
            READ_INT(store, &id);
        }
        else {
            READ_STR(store, pbuf, sizeof(pbuf));
            id = atoi36(pbuf);
        }
    }
}

void write_alliances(struct gamedata *data)
{
    alliance *al = alliances;
    while (al) {
        if (al->_leader) {
            WRITE_INT(data->store, al->id);
            WRITE_STR(data->store, al->name);
            WRITE_INT(data->store, (int)al->flags);
            write_faction_reference(al->_leader, data->store);
            WRITE_SECTION(data->store);
        }
        al = al->next;
    }
    WRITE_INT(data->store, 0);
    WRITE_SECTION(data->store);
}

void write_items(struct storage *store, item * ilist)
{
    item *itm;
    for (itm = ilist; itm; itm = itm->next) {
        assert(itm->number >= 0);
        if (itm->number) {
            WRITE_TOK(store, resourcename(itm->type->rtype, 0));
            WRITE_INT(store, itm->number);
        }
    }
    WRITE_TOK(store, "end");
}

static int resolve_owner(variant id, void *address)
{
    region_owner *owner = (region_owner *)address;
    int result = 0;
    faction *f = NULL;
    if (id.i != 0) {
        f = findfaction(id.i);
        if (f == NULL) {
            log_error("region has an invalid owner (%s)\n", itoa36(id.i));
        }
    }
    owner->owner = f;
    if (f) {
        owner->alliance = f->alliance;
    }
    return result;
}

static void read_owner(struct gamedata *data, region_owner ** powner)
{
    int since_turn;

    READ_INT(data->store, &since_turn);
    if (since_turn >= 0) {
        region_owner *owner = malloc(sizeof(region_owner));
        owner->since_turn = since_turn;
        READ_INT(data->store, &owner->morale_turn);
        if (data->version >= MOURNING_VERSION) {
            READ_INT(data->store, &owner->flags);
        }
        else {
            owner->flags = 0;
        }
        if (data->version >= OWNER_2_VERSION) {
            int id;
            READ_INT(data->store, &id);
            owner->alliance = id ? findalliance(id) : NULL;
        }
        else {
            owner->alliance = NULL;
        }
        read_reference(owner, data->store, &read_faction_reference, &resolve_owner);
        *powner = owner;
    }
    else {
        *powner = 0;
    }
}

static void write_owner(struct gamedata *data, region_owner * owner)
{
    if (owner) {
        WRITE_INT(data->store, owner->since_turn);
        WRITE_INT(data->store, owner->morale_turn);
        WRITE_INT(data->store, owner->flags);
        WRITE_INT(data->store, owner->alliance ? owner->alliance->id : 0);
        write_faction_reference(owner->owner, data->store);
    }
    else {
        WRITE_INT(data->store, -1);
    }
}

int current_turn(void)
{
    char zText[MAX_PATH];
    int cturn = 0;
    FILE *F;

    sprintf(zText, "%s/turn", basepath());
    F = fopen(zText, "r");
    if (!F) {
        perror(zText);
    }
    else {
        int c = fscanf(F, "%d\n", &cturn);
        fclose(F);
        if (c != 1) {
            return -1;
        }
    }
    return cturn;
}

static void
writeorder(struct gamedata *data, const struct order *ord,
const struct locale *lang)
{
    char obuf[1024];
    write_order(ord, obuf, sizeof(obuf));
    if (obuf[0])
        WRITE_STR(data->store, obuf);
}

unit *read_unit(struct gamedata *data)
{
    unit *u;
    int number, n, p;
    order **orderp;
    char obuf[DISPLAYSIZE];
    faction *f;
    char rname[32];

    READ_INT(data->store, &n);
    if (n <= 0) {
        log_error("data contains invalid unit %d.\n", n);
        assert(n > 0);
        return 0;
    }
    u = findunit(n);
    if (u) {
        log_error("reading unit %s that already exists.\n", unitname(u));
        while (u->attribs) {
            a_remove(&u->attribs, u->attribs);
        }
        while (u->items) {
            i_free(i_remove(&u->items, u->items));
        }
        free(u->skills);
        u->skills = 0;
        u->skill_size = 0;
        u_setfaction(u, NULL);
    }
    else {
        u = calloc(sizeof(unit), 1);
        u->no = n;
        uhash(u);
    }

    READ_INT(data->store, &n);
    f = findfaction(n);
    if (f != u->faction) {
        u_setfaction(u, f);
    }
    if (u->faction) {
        ++u->faction->no_units;
    }
    else {
        log_error("unit %s has faction == NULL\n", itoa36(u->no));
        return 0;
    }

    READ_STR(data->store, obuf, sizeof(obuf));
    u->_name = obuf[0] ? _strdup(obuf) : 0;
    if (lomem) {
        READ_STR(data->store, NULL, 0);
    }
    else {
        READ_STR(data->store, obuf, sizeof(obuf));
        u->display = obuf[0] ? _strdup(obuf) : 0;
    }
    READ_INT(data->store, &number);
    set_number(u, number);

    READ_INT(data->store, &n);
    u->age = (short)n;

    READ_TOK(data->store, rname, sizeof(rname));
    u_setrace(u, rc_find(rname));

    READ_TOK(data->store, rname, sizeof(rname));
    if (rname[0] && skill_enabled(SK_STEALTH))
        u->irace = rc_find(rname);
    else
        u->irace = NULL;

    if (u_race(u)->describe) {
        const char *rcdisp = u_race(u)->describe(u, u->faction->locale);
        if (u->display && rcdisp) {
            /* see if the data file contains old descriptions */
            if (strcmp(rcdisp, u->display) == 0) {
                free(u->display);
                u->display = NULL;
            }
        }
    }

    READ_INT(data->store, &n);
    if (n > 0) {
        building * b = findbuilding(n);
        if (b) {
            u_set_building(u, b);
            if (fval(u, UFL_OWNER)) {
                building_set_owner(u);
            }
        }
        else {
            log_error("read_unit: unit in unkown building '%s'\n", itoa36(n));
        }
    }

    READ_INT(data->store, &n);
    if (n > 0) {
        ship * sh = findship(n);
        if (sh) {
            u_set_ship(u, sh);
            if (fval(u, UFL_OWNER)) {
                ship_set_owner(u);
            }
        }
        else {
            log_error("read_unit: unit in unkown ship '%s'\n", itoa36(n));
        }
    }

    READ_INT(data->store, &n);
    setstatus(u, n);
    READ_INT(data->store, &u->flags);
    u->flags &= UFL_SAVEMASK;
    if ((u->flags & UFL_ANON_FACTION) && !rule_stealth_anon()) {
        /* if this rule is broken, then fix broken units */
        u->flags -= UFL_ANON_FACTION;
        log_warning("%s was anonymous.\n", unitname(u));
    }
    /* Persistente Befehle einlesen */
    free_orders(&u->orders);
    READ_STR(data->store, obuf, sizeof(obuf));
    p = n = 0;
    orderp = &u->orders;
    while (obuf[0]) {
        if (!lomem) {
            order *ord = parse_order(obuf, u->faction->locale);
            if (ord != NULL) {
                if (++n < MAXORDERS) {
                    if (!is_persistent(ord) || ++p < MAXPERSISTENT) {
                        *orderp = ord;
                        orderp = &ord->next;
                        ord = NULL;
                    }
                    else if (p == MAXPERSISTENT) {
                        log_warning("%s had %d or more persistent orders\n", unitname(u), MAXPERSISTENT);
                    }
                }
                else if (n == MAXORDERS) {
                    log_warning("%s had %d or more orders\n", unitname(u), MAXORDERS);
                }
                if (ord != NULL)
                    free_order(ord);
            }
        }
        READ_STR(data->store, obuf, sizeof(obuf));
    }
    set_order(&u->thisorder, NULL);

    assert(u_race(u));
    for (;;) {
        int n, level, weeks;
        skill_t sk;
        READ_INT(data->store, &n);
        sk = (skill_t)n;
        if (sk == NOSKILL) break;
        READ_INT(data->store, &level);
        READ_INT(data->store, &weeks);
        if (level) {
            skill *sv = add_skill(u, sk);
            sv->level = sv->old = (unsigned char)level;
            sv->weeks = (unsigned char)weeks;
        }
    }
    read_items(data->store, &u->items);
    READ_INT(data->store, &u->hp);
    if (u->hp < u->number) {
        log_error("Einheit %s hat %u Personen, und %u Trefferpunkte\n", itoa36(u->no), u->number, u->hp);
        u->hp = u->number;
    }

    a_read(data->store, &u->attribs, u);
    return u;
}

void write_unit(struct gamedata *data, const unit * u)
{
    order *ord;
    int i, p = 0;
    unsigned int flags = u->flags & UFL_SAVEMASK;
    const race *irace = u_irace(u);

    write_unit_reference(u, data->store);
    write_faction_reference(u->faction, data->store);
    WRITE_STR(data->store, u->_name);
    WRITE_STR(data->store, u->display ? u->display : "");
    WRITE_INT(data->store, u->number);
    WRITE_INT(data->store, u->age);
    WRITE_TOK(data->store, u_race(u)->_name);
    WRITE_TOK(data->store, (irace && irace != u_race(u)) ? irace->_name : "");
    write_building_reference(u->building, data->store);
    write_ship_reference(u->ship, data->store);
    WRITE_INT(data->store, u->status);
    if (u->building && u == building_owner(u->building)) flags |= UFL_OWNER;
    if (u->ship && u == ship_owner(u->ship)) flags |= UFL_OWNER;
    WRITE_INT(data->store, flags);
    WRITE_SECTION(data->store);
    for (ord = u->old_orders; ord; ord = ord->next) {
        if (++p < MAXPERSISTENT) {
            writeorder(data, ord, u->faction->locale);
        }
        else {
            log_warning("%s had %d or more persistent orders\n", unitname(u), MAXPERSISTENT);
            break;
        }
    }
    for (ord = u->orders; ord; ord = ord->next) {
        keyword_t kwd = getkeyword(ord);
        if (u->old_orders && is_repeated(kwd))
            continue;                 /* has new defaults */
        if (is_persistent(ord)) {
            if (++p < MAXPERSISTENT) {
                writeorder(data, ord, u->faction->locale);
            }
            else {
                log_warning("%s had %d or more persistent orders\n", unitname(u), MAXPERSISTENT);
                break;
            }
        }
    }
    /* write an empty string to terminate the list */
    WRITE_STR(data->store, "");
    WRITE_SECTION(data->store);

    assert(u_race(u));

    for (i = 0; i != u->skill_size; ++i) {
        skill *sv = u->skills + i;
        assert(sv->weeks <= sv->level * 2 + 1);
        if (sv->level > 0) {
            WRITE_INT(data->store, sv->id);
            WRITE_INT(data->store, sv->level);
            WRITE_INT(data->store, sv->weeks);
        }
    }
    WRITE_INT(data->store, -1);
    WRITE_SECTION(data->store);
    write_items(data->store, u->items);
    WRITE_SECTION(data->store);
    if (u->hp == 0) {
        log_error("unit %s has 0 hitpoints, adjusting.\n", itoa36(u->no));
        ((unit *)u)->hp = u->number;
    }
    WRITE_INT(data->store, u->hp);
    WRITE_SECTION(data->store);
    a_write(data->store, u->attribs, u);
    WRITE_SECTION(data->store);
}

static region *readregion(struct gamedata *data, int x, int y)
{
    region *r = findregion(x, y);
    const terrain_type *terrain;
    char name[NAMESIZE];
    int uid = 0;
    int n;

    READ_INT(data->store, &uid);

    if (r == NULL) {
        plane *pl = findplane(x, y);
        r = new_region(x, y, pl, uid);
    }
    else {
        assert(uid == 0 || r->uid == uid);
        current_region = r;
        while (r->attribs)
            a_remove(&r->attribs, r->attribs);
        if (r->land) {
            free_land(r->land);
            r->land = 0;
        }
        while (r->resources) {
            rawmaterial *rm = r->resources;
            r->resources = rm->next;
            free(rm);
        }
        r->land = 0;
    }
    if (lomem) {
        READ_STR(data->store, NULL, 0);
    }
    else {
        char info[DISPLAYSIZE];
        READ_STR(data->store, info, sizeof(info));
        region_setinfo(r, info);
    }

    READ_STR(data->store, name, sizeof(name));
    terrain = get_terrain(name);
    if (terrain == NULL) {
        log_error("Unknown terrain '%s'\n", name);
        assert(!"unknown terrain");
    }
    r->terrain = terrain;
    READ_INT(data->store, &r->flags);
    READ_INT(data->store, &n);
    r->age = (unsigned short)n;

    if (fval(r->terrain, LAND_REGION)) {
        r->land = calloc(1, sizeof(land_region));
        READ_STR(data->store, name, sizeof(name));
        r->land->name = _strdup(name);
    }
    if (r->land) {
        int i;
        rawmaterial **pres = &r->resources;

        READ_INT(data->store, &i);
        if (i < 0) {
            log_error("number of trees in %s is %d.\n", regionname(r, NULL), i);
            i = 0;
        }
        rsettrees(r, 0, i);
        READ_INT(data->store, &i);
        if (i < 0) {
            log_error("number of young trees in %s is %d.\n", regionname(r, NULL), i);
            i = 0;
        }
        rsettrees(r, 1, i);
        READ_INT(data->store, &i);
        if (i < 0) {
            log_error("number of seeds in %s is %d.\n", regionname(r, NULL), i);
            i = 0;
        }
        rsettrees(r, 2, i);

        READ_INT(data->store, &i);
        rsethorses(r, i);
        assert(*pres == NULL);
        for (;;) {
            rawmaterial *res;
            READ_STR(data->store, name, sizeof(name));
            if (strcmp(name, "end") == 0)
                break;
            res = malloc(sizeof(rawmaterial));
            res->type = rmt_find(name);
            if (res->type == NULL) {
                log_error("invalid resourcetype %s in data.\n", name);
            }
            assert(res->type != NULL);
            READ_INT(data->store, &n);
            res->level = n;
            READ_INT(data->store, &n);
            res->amount = n;
            res->flags = 0;

            READ_INT(data->store, &n);
            res->startlevel = n;
            READ_INT(data->store, &n);
            res->base = n;
            READ_INT(data->store, &n);
            res->divisor = n;

            *pres = res;
            pres = &res->next;
        }
        *pres = NULL;

        READ_STR(data->store, name, sizeof(name));
        if (strcmp(name, "noherb") != 0) {
            const resource_type *rtype = rt_find(name);
            assert(rtype && rtype->itype && fval(rtype->itype, ITF_HERB));
            rsetherbtype(r, rtype->itype);
        }
        else {
            rsetherbtype(r, NULL);
        }
        READ_INT(data->store, &n);
        rsetherbs(r, (short)n);
        READ_INT(data->store, &n);
        rsetpeasants(r, n);
        READ_INT(data->store, &n);
        rsetmoney(r, n);
    }

    assert(r->terrain != NULL);
    assert(rhorses(r) >= 0);
    assert(rpeasants(r) >= 0);
    assert(rmoney(r) >= 0);

    if (r->land) {
        int n;
        for (;;) {
            const struct resource_type *rtype;
            READ_STR(data->store, name, sizeof(name));
            if (!strcmp(name, "end"))
                break;
            rtype = rt_find(name);
            assert(rtype && rtype->ltype);
            READ_INT(data->store, &n);
            r_setdemand(r, rtype->ltype, n);
        }
        read_items(data->store, &r->land->items);
        if (data->version >= REGIONOWNER_VERSION) {
            READ_INT(data->store, &n);
            r->land->morale = (short)n;
            if (r->land->morale < 0) {
                r->land->morale = 0;
            }
            read_owner(data, &r->land->ownership);
        }
    }
    a_read(data->store, &r->attribs, r);
    return r;
}

void writeregion(struct gamedata *data, const region * r)
{
    assert(r);
    assert(data);

    WRITE_INT(data->store, r->uid);
    WRITE_STR(data->store, region_getinfo(r));
    WRITE_TOK(data->store, r->terrain->_name);
    WRITE_INT(data->store, r->flags & RF_SAVEMASK);
    WRITE_INT(data->store, r->age);
    WRITE_SECTION(data->store);
    if (fval(r->terrain, LAND_REGION)) {
        const item_type *rht;
        struct demand *demand;
        rawmaterial *res = r->resources;
        
        assert(r->land);
        WRITE_STR(data->store, (const char *)r->land->name);
        assert(rtrees(r, 0) >= 0);
        assert(rtrees(r, 1) >= 0);
        assert(rtrees(r, 2) >= 0);
        WRITE_INT(data->store, rtrees(r, 0));
        WRITE_INT(data->store, rtrees(r, 1));
        WRITE_INT(data->store, rtrees(r, 2));
        WRITE_INT(data->store, rhorses(r));

        while (res) {
            WRITE_TOK(data->store, res->type->name);
            WRITE_INT(data->store, res->level);
            WRITE_INT(data->store, res->amount);
            WRITE_INT(data->store, res->startlevel);
            WRITE_INT(data->store, res->base);
            WRITE_INT(data->store, res->divisor);
            res = res->next;
        }
        WRITE_TOK(data->store, "end");

        rht = rherbtype(r);
        if (rht) {
            WRITE_TOK(data->store, resourcename(rht->rtype, 0));
        }
        else {
            WRITE_TOK(data->store, "noherb");
        }
        WRITE_INT(data->store, rherbs(r));
        WRITE_INT(data->store, rpeasants(r));
        WRITE_INT(data->store, rmoney(r));
        for (demand = r->land->demands; demand; demand = demand->next) {
            WRITE_TOK(data->store, resourcename(demand->type->itype->rtype, 0));
            WRITE_INT(data->store, demand->value);
        }
        WRITE_TOK(data->store, "end");
        write_items(data->store, r->land->items);
        WRITE_SECTION(data->store);
#if RELEASE_VERSION>=REGIONOWNER_VERSION
        WRITE_INT(data->store, r->land->morale);
        write_owner(data, r->land->ownership);
        WRITE_SECTION(data->store);
#endif
    }
    a_write(data->store, r->attribs, r);
    WRITE_SECTION(data->store);
}

static ally **addally(const faction * f, ally ** sfp, int aid, int state)
{
    struct faction *af = findfaction(aid);
    ally *sf;

    state &= ~HELP_OBSERVE;
#ifndef REGIONOWNERS
    state &= ~HELP_TRAVEL;
#endif
    state &= HelpMask();

    if (state == 0)
        return sfp;

    while (*sfp) {
        sfp = &(*sfp)->next;
    }

    sf = ally_add(sfp, af);
    if (!sf->faction) {
        variant id;
        id.i = aid;
        ur_add(id, &sf->faction, resolve_faction);
    }
    sf->status = state & HELP_ALL;

    return &sf->next;
}

int get_spell_level_faction(const spell * sp, void * cbdata)
{
    static spellbook * common = 0;
    spellbook * book;
    faction * f = (faction *)cbdata;
    spellbook_entry * sbe;

    book = get_spellbook(magic_school[f->magiegebiet]);
    if (book) {
        sbe = spellbook_get(book, sp);
        if (sbe) return sbe->level;
    }
    if (!common) {
        common = get_spellbook(magic_school[M_COMMON]);
    }
    sbe = spellbook_get(common, sp);
    if (sbe) {
        return sbe->level;
    }
    else {
        log_error("read_spellbook: faction '%s' has a spell with unknown level: '%s'", factionname(f), sp->sname);
    }
    return 0;
}

void read_spellbook(spellbook **bookp, struct storage *store, int(*get_level)(const spell * sp, void *), void * cbdata)
{
    for (;;) {
        spell *sp = 0;
        char spname[64];
        int level = 0;

        READ_TOK(store, spname, sizeof(spname));
        if (strcmp(spname, "end") == 0)
            break;
        if (bookp) {
            sp = find_spell(spname);
            if (!sp) {
                log_error("read_spells: could not find spell '%s'\n", spname);
            }
        }
        if (global.data_version >= SPELLBOOK_VERSION) {
            READ_INT(store, &level);
        }
        if (sp) {
            spellbook * sb = *bookp;
            if (level <= 0 && get_level) {
                level = get_level(sp, cbdata);
            }
            if (!sb) {
                *bookp = create_spellbook(0);
                sb = *bookp;
            }
            if (level>0 && (global.data_version >= SPELLBOOK_VERSION || !spellbook_get(sb, sp))) {
                spellbook_add(sb, sp, level);
            }
        }
    }
}

void write_spellbook(const struct spellbook *book, struct storage *store)
{
    quicklist *ql;
    int qi;

    if (book) {
        for (ql = book->spells, qi = 0; ql; ql_advance(&ql, &qi, 1)) {
            spellbook_entry *sbe = (spellbook_entry *)ql_get(ql, qi);
            WRITE_TOK(store, sbe->sp->sname);
            WRITE_INT(store, sbe->level);
        }
    }
    WRITE_TOK(store, "end");
}

/** Reads a faction from a file.
 * This function requires no context, can be called in any state. The
 * faction may not already exist, however.
 */
faction *readfaction(struct gamedata * data)
{
    ally **sfp;
    int planes, n;
    faction *f;
    char name[DISPLAYSIZE];

    READ_INT(data->store, &n);
    f = findfaction(n);
    if (f == NULL) {
        f = (faction *)calloc(1, sizeof(faction));
        f->no = n;
    }
    else {
        f->allies = NULL;           /* mem leak */
        while (f->attribs)
            a_remove(&f->attribs, f->attribs);
    }
    READ_INT(data->store, &f->subscription);

    if (data->version >= SPELL_LEVEL_VERSION) {
        READ_INT(data->store, &f->max_spelllevel);
    }
    if (alliances || data->version >= OWNER_2_VERSION) {
        int allianceid;
        READ_INT(data->store, &allianceid);
        if (allianceid > 0)
            f->alliance = findalliance(allianceid);
        if (f->alliance) {
            alliance *al = f->alliance;
            if (al->flags & ALF_NON_ALLIED) {
                assert(!al->members
                    || !"non-allied dummy-alliance has more than one member");
            }
            ql_push(&al->members, f);
        }
        else if (rule_region_owners()) {
            /* compat fix for non-allied factions */
            alliance *al = makealliance(0, NULL);
            setalliance(f, al);
        }
        if (data->version >= OWNER_2_VERSION) {
            READ_INT(data->store, &f->alliance_joindate);
        }
        else {
            f->alliance_joindate = turn - 10; /* we're guessing something safe here */
        }
    }

    READ_STR(data->store, name, sizeof(name));
    f->name = _strdup(name);
    READ_STR(data->store, name, sizeof(name));
    f->banner = _strdup(name);

    log_debug("   - Lese Partei %s (%s)", f->name, factionid(f));

    READ_STR(data->store, name, sizeof(name));
    if (set_email(&f->email, name) != 0) {
        log_warning("Invalid email address for faction %s: %s\n", itoa36(f->no), name);
        set_email(&f->email, "");
    }

    READ_STR(data->store, name, sizeof(name));
    f->passw = _strdup(name);
    if (data->version < NOOVERRIDE_VERSION) {
        READ_STR(data->store, 0, 0);
    }

    READ_STR(data->store, name, sizeof(name));
    f->locale = get_locale(name);
    if (!f->locale) f->locale = default_locale;
    READ_INT(data->store, &f->lastorders);
    READ_INT(data->store, &f->age);
    READ_STR(data->store, name, sizeof(name));
    f->race = rc_find(name);
    if (!f->race) {
        log_error("unknown race in data: %s\n", name);
    }
    assert(f->race);
    READ_INT(data->store, &n);
    f->magiegebiet = (magic_t)n;

    if (data->version < FOSS_VERSION) {
        /* ignore karma */
        READ_INT(data->store, &n);
    }

    READ_INT(data->store, &f->flags);
    if (data->version < INTFLAGS_VERSION) {
        if (f->no == 0 || f->no == 666) {
            f->flags = FFL_NPC | FFL_NOIDLEOUT;
        }
    }

    a_read(data->store, &f->attribs, f);
    read_items(data->store, &f->items);
    for (;;) {
        READ_TOK(data->store, name, sizeof(name));
        if (strcmp("end", name) == 0)
            break;
        READ_INT(data->store, &n); /* there used to be a level here, which is now ignored */
    }
    READ_INT(data->store, &planes);
    while (--planes >= 0) {
        int id, ux, uy;
        READ_INT(data->store, &id);
        READ_INT(data->store, &ux);
        READ_INT(data->store, &uy);
        faction_setorigin(f, id, ux, uy);
    }
    f->newbies = 0;

    READ_INT(data->store, &n);
    f->options = n;

    n = want(O_REPORT) | want(O_COMPUTER);
    if ((f->options & n) == 0) {
        /* Kein Report eingestellt, Fehler */
        f->options |= n;
    }
    if (data->version < JSON_REPORT_VERSION) {
        /* mistakes were made in the past*/
        f->options &= ~want(O_JSON);
    }
    sfp = &f->allies;
    for (;;) {
        int aid = 0;
        READ_INT(data->store, &aid);
        if (aid > 0) {
            int state;
            READ_INT(data->store, &state);
            sfp = addally(f, sfp, aid, state);
        }
        else {
            break;
        }
    }
    read_groups(data->store, f);
    f->spellbook = 0;
    if (data->version >= REGIONOWNER_VERSION) {
        read_spellbook(FactionSpells() ? &f->spellbook : 0, data->store, get_spell_level_faction, (void *)f);
    }
    return f;
}

void writefaction(struct gamedata *data, const faction * f)
{
    ally *sf;
    ursprung *ur;

    write_faction_reference(f, data->store);
    WRITE_INT(data->store, f->subscription);
#if RELEASE_VERSION >= SPELL_LEVEL_VERSION
    WRITE_INT(data->store, f->max_spelllevel);
#endif
    if (f->alliance) {
        WRITE_INT(data->store, f->alliance->id);
        if (f->alliance->flags & ALF_NON_ALLIED) {
            assert(f == f->alliance->_leader
                || !"non-allied faction is not leader of its own dummy-alliance.");
        }
    }
    else {
        WRITE_INT(data->store, 0);
    }
    WRITE_INT(data->store, f->alliance_joindate);

    WRITE_STR(data->store, (const char *)f->name);
    WRITE_STR(data->store, (const char *)f->banner);
    WRITE_STR(data->store, f->email);
    WRITE_TOK(data->store, (const char *)f->passw);
    WRITE_TOK(data->store, locale_name(f->locale));
    WRITE_INT(data->store, f->lastorders);
    WRITE_INT(data->store, f->age);
    WRITE_TOK(data->store, f->race->_name);
    WRITE_SECTION(data->store);
    WRITE_INT(data->store, f->magiegebiet);

    WRITE_INT(data->store, f->flags & FFL_SAVEMASK);
    a_write(data->store, f->attribs, f);
    WRITE_SECTION(data->store);
    write_items(data->store, f->items);
    WRITE_SECTION(data->store);
    WRITE_TOK(data->store, "end");
    WRITE_SECTION(data->store);
    WRITE_INT(data->store, listlen(f->ursprung));
    for (ur = f->ursprung; ur; ur = ur->next) {
        WRITE_INT(data->store, ur->id);
        WRITE_INT(data->store, ur->x);
        WRITE_INT(data->store, ur->y);
    }
    WRITE_SECTION(data->store);
    WRITE_INT(data->store, f->options & ~want(O_DEBUG));
    WRITE_SECTION(data->store);

    for (sf = f->allies; sf; sf = sf->next) {
        int no;
        int status;

        assert(sf->faction);

        no = sf->faction->no;
        status = alliedfaction(NULL, f, sf->faction, HELP_ALL);

        if (status != 0) {
            WRITE_INT(data->store, no);
            WRITE_INT(data->store, sf->status);
        }
    }
    WRITE_INT(data->store, 0);
    WRITE_SECTION(data->store);
    write_groups(data->store, f);
    write_spellbook(f->spellbook, data->store);
}

static int cb_sb_maxlevel(spellbook_entry *sbe, void *cbdata) {
    faction *f = (faction *)cbdata;
    if (sbe->level > f->max_spelllevel) {
        f->max_spelllevel = sbe->level;
    }
    return 0;
}

int readgame(const char *filename, bool backup)
{
    int n, p, nread;
    faction *f, **fp;
    region *r;
    building *b, **bp;
    ship **shp;
    unit *u;
    int rmax = maxregions;
    char path[MAX_PATH];
    char name[DISPLAYSIZE];
    const struct building_type *bt_lighthouse = bt_find("lighthouse");
    gamedata gdata = { 0 };
    storage store;
    stream strm;
    FILE *F;

    init_locales();
    log_debug("- reading game data from %s\n", filename);
    sprintf(path, "%s/%s", datapath(), filename);

    if (backup) {
        create_backup(path);
    }

    F = fopen(path, "rb");
    if (!F) {
        perror(path);
        return -1;
    }
    fread(&gdata.version, sizeof(int), 1, F);
    if (gdata.version >= INTPAK_VERSION) {
        int stream_version;
        size_t sz = fread(&stream_version, sizeof(int), 1, F);
        assert((sz==1 && stream_version == STREAM_VERSION) || !"unsupported data format");
    }
    assert(gdata.version >= MIN_VERSION || !"unsupported data format");
    assert(gdata.version <= MAX_VERSION || !"unsupported data format");

    gdata.encoding = enc_gamedata;
    fstream_init(&strm, F);
    binstore_init(&store, &strm);
    gdata.store = &store;
    global.data_version = gdata.version; /* HACK: attribute::read does not have access to gamedata, only storage */

    if (gdata.version >= BUILDNO_VERSION) {
        int build;
        READ_INT(&store, &build);
        log_debug("data in %s created with build %d.", filename, build);
    }
    if (gdata.version >= SAVEGAMEID_VERSION) {
        int gameid;

        READ_INT(&store, &gameid);
        if (gameid != game_id()) {
            int c;
            log_warning("game mismatch: datafile contains game %d, but config is for %d", gameid, game_id());
            printf("WARNING: invalid game id. any key to continue, Ctrl-C to stop\n");
            c = getchar();
            if (c == EOF) {
                log_error("aborting.");
                abort();
            }
        }
    }
    else {
        READ_STR(&store, NULL, 0);
    }
    a_read(&store, &global.attribs, NULL);
    READ_INT(&store, &turn);
    global.data_turn = turn;
    log_debug(" - reading turn %d\n", turn);
    rng_init(turn);
    ++global.cookie;
    READ_INT(&store, &nread);          /* max_unique_id = ignore */
    READ_INT(&store, &nextborder);

    /* Planes */
    planes = NULL;
    READ_INT(&store, &nread);
    while (--nread >= 0) {
        int id;
        variant fno;
        plane *pl;

        READ_INT(&store, &id);
        pl = getplanebyid(id);

        if (pl == NULL) {
            pl = calloc(1, sizeof(plane));
        }
        else {
            log_warning("the plane with id=%d already exists.\n", id);
        }
        pl->id = id;
        READ_STR(&store, name, sizeof(name));
        pl->name = _strdup(name);
        READ_INT(&store, &pl->minx);
        READ_INT(&store, &pl->maxx);
        READ_INT(&store, &pl->miny);
        READ_INT(&store, &pl->maxy);
        READ_INT(&store, &pl->flags);

        /* read watchers */
        if (gdata.version < FIX_WATCHERS_VERSION) {
            char rname[64];
            /* before this version, watcher storage was pretty broken. we are incompatible and don't read them */
            for (;;) {
                READ_TOK(&store, rname, sizeof(rname));
                if (strcmp(rname, "end") == 0) {
                    break;                /* this is most likely the end of the list */
                }
                else {
                    log_error(
                        ("This datafile contains watchers, but we are unable to read them\n"));
                }
            }
        }
        else {
            fno = read_faction_reference(&store);
            while (fno.i) {
                watcher *w = (watcher *)malloc(sizeof(watcher));
                ur_add(fno, &w->faction, resolve_faction);
                READ_INT(&store, &n);
                w->mode = (unsigned char)n;
                w->next = pl->watchers;
                pl->watchers = w;
                fno = read_faction_reference(&store);
            }
        }
        a_read(&store, &pl->attribs, pl);
        if (pl->id != 1094969858) { // Regatta
            addlist(&planes, pl);
        }
    }

    /* Read factions */
    read_alliances(&store);
    READ_INT(&store, &nread);
    log_debug(" - Einzulesende Parteien: %d\n", nread);
    fp = &factions;
    while (*fp)
        fp = &(*fp)->next;

    while (--nread >= 0) {
        faction *f = readfaction(&gdata);

        *fp = f;
        fp = &f->next;
        fhash(f);
    }
    *fp = 0;

    /* Regionen */

    READ_INT(&store, &nread);
    assert(nread < MAXREGIONS);
    if (rmax < 0) {
        rmax = nread;
    }
    log_debug(" - Einzulesende Regionen: %d/%d\r", rmax, nread);
    while (--nread >= 0) {
        unit **up;
        int x, y;
        READ_INT(&store, &x);
        READ_INT(&store, &y);

        if ((nread & 0x3FF) == 0) {     /* das spart extrem Zeit */
            log_debug(" - Einzulesende Regionen: %d/%d * %d,%d    \r", rmax, nread, x, y);
        }
        --rmax;

        r = readregion(&gdata, x, y);

        /* Burgen */
        READ_INT(&store, &p);
        bp = &r->buildings;

        while (--p >= 0) {

            b = (building *)calloc(1, sizeof(building));
            READ_INT(&store, &b->no);
            *bp = b;
            bp = &b->next;
            bhash(b);
            READ_STR(&store, name, sizeof(name));
            b->name = _strdup(name);
            if (lomem) {
                READ_STR(gdata.store, NULL, 0);
            }
            else {
                READ_STR(&store, name, sizeof(name));
                b->display = _strdup(name);
            }
            READ_INT(&store, &b->size);
            READ_STR(&store, name, sizeof(name));
            b->type = bt_find(name);
            b->region = r;
            a_read(&store, &b->attribs, b);
            if (b->type == bt_lighthouse) {
                r->flags |= RF_LIGHTHOUSE;
            }
        }
        /* Schiffe */

        READ_INT(&store, &p);
        shp = &r->ships;

        while (--p >= 0) {
            ship *sh = (ship *)calloc(1, sizeof(ship));
            sh->region = r;
            READ_INT(&store, &sh->no);
            *shp = sh;
            shp = &sh->next;
            shash(sh);
            READ_STR(&store, name, sizeof(name));
            sh->name = _strdup(name);
            if (lomem) {
                READ_STR(&store, NULL, 0);
            }
            else {
                READ_STR(&store, name, sizeof(name));
                sh->display = _strdup(name);
            }
            READ_STR(&store, name, sizeof(name));
            sh->type = st_find(name);
            if (sh->type == NULL) {
                /* old datafiles */
                sh->type = st_find((const char *)LOC(default_locale, name));
            }
            assert(sh->type || !"ship_type not registered!");

            READ_INT(&store, &sh->size);
            READ_INT(&store, &sh->damage);
            if (gdata.version >= FOSS_VERSION) {
                READ_INT(&store, &sh->flags);
            }

            /* Attribute rekursiv einlesen */

            READ_INT(&store, &n);
            sh->coast = (direction_t)n;
            if (sh->type->flags & SFL_NOCOAST) {
                sh->coast = NODIRECTION;
            }
            a_read(&store, &sh->attribs, sh);
        }

        *shp = 0;

        /* Einheiten */

        READ_INT(&store, &p);
        up = &r->units;

        while (--p >= 0) {
            unit *u = read_unit(&gdata);

            if (gdata.version < JSON_REPORT_VERSION) {
                if (u->_name && fval(u->faction, FFL_NPC)) {
                    if (!u->_name[0] || unit_name_equals_race(u)) {
                        unit_setname(u, NULL);
                    }
                }
            }
            assert(u->region == NULL);
            u->region = r;
            *up = u;
            up = &u->next;

            update_interval(u->faction, u->region);
        }
    }
    read_borders(&store);

    binstore_done(&store);
    fstream_done(&strm);
    /* Unaufgeloeste Zeiger initialisieren */
    log_debug("fixing unresolved references.\n");
    resolve();

    log_debug("updating area information for lighthouses.\n");
    for (r = regions; r; r = r->next) {
        if (r->flags & RF_LIGHTHOUSE) {
            building *b;
            for (b = r->buildings; b; b = b->next)
                update_lighthouse(b);
        }
    }
    log_debug("marking factions as alive.\n");
    for (f = factions; f; f = f->next) {
        if (f->flags & FFL_NPC) {
            f->alive = 1;
            f->magiegebiet = M_GRAY;
            if (f->no == 0) {
                int no = 666;
                while (findfaction(no))
                    ++no;
                log_warning("renum(monsters, %d)\n", no);
                renumber_faction(f, no);
            }
        }
        else {
            for (u = f->units; u; u = u->nextF) {
                if (global.data_version < SPELL_LEVEL_VERSION) {
                    sc_mage *mage = get_mage(u);
                    if (mage) {
                        faction *f = u->faction;
                        int skl = effskill(u, SK_MAGIC, 0);
                        if (f->magiegebiet == M_GRAY) {
                            log_error("faction %s had magic=gray, fixing (%s)\n", factionname(f), magic_school[mage->magietyp]);
                            f->magiegebiet = mage->magietyp;
                        }
                        if (f->max_spelllevel < skl) {
                            f->max_spelllevel = skl;
                        }
                        if (mage->spellcount < 0) {
                            mage->spellcount = 0;
                        }
                    }
                }
                if (u->number > 0) {
                    f->alive = true;
                    if (global.data_version >= SPELL_LEVEL_VERSION) {
                        break;
                    }
                }
            }
            if (global.data_version < SPELL_LEVEL_VERSION && f->spellbook) {
                spellbook_foreach(f->spellbook, cb_sb_maxlevel, f);
            }
        }
    }
    if (loadplane || maxregions >= 0) {
        remove_empty_factions();
    }
    log_debug("Done loading turn %d.\n", turn);

    return 0;
}

static void clear_npc_orders(faction *f)
{
    if (f) {
        unit *u;
        for (u = f->units; u; u = u->nextF) {
            free_orders(&u->orders);
        }
    }
}

int writegame(const char *filename)
{
    int n;
    faction *f;
    region *r;
    building *b;
    ship *sh;
    unit *u;
    plane *pl;
    char path[MAX_PATH];
    gamedata gdata;
    storage store;
    stream strm;
    FILE *F;

    sprintf(path, "%s/%s", datapath(), filename);
#ifdef HAVE_UNISTD_H
    /* make sure we don't overwrite an existing file (hard links) */
    unlink(path);
#endif
    F = fopen(path, "wb");
    if (!F) {
        /* we might be missing the directory, let's try creating it */
        int err = _mkdir(datapath());
        if (err) return err;
        F = fopen(path, "wb");
        if (!F) {
            perror(path);
            return -1;
        }
    }

    gdata.store = &store;
    gdata.encoding = enc_gamedata;
    gdata.version = RELEASE_VERSION;
    global.data_version = RELEASE_VERSION;
    n = STREAM_VERSION;
    fwrite(&gdata.version, sizeof(int), 1, F);
    fwrite(&n, sizeof(int), 1, F);

    fstream_init(&strm, F);
    binstore_init(&store, &strm);

    /* globale Variablen */

    WRITE_INT(&store, VERSION_BUILD);
    WRITE_INT(&store, game_id());
    WRITE_SECTION(&store);

    a_write(&store, global.attribs, NULL);
    WRITE_SECTION(&store);

    WRITE_INT(&store, turn);
    WRITE_INT(&store, 0 /*max_unique_id */);
    WRITE_INT(&store, nextborder);

    /* Write planes */
    WRITE_SECTION(&store);
    WRITE_INT(&store, listlen(planes));
    WRITE_SECTION(&store);

    for (pl = planes; pl; pl = pl->next) {
        watcher *w;
        WRITE_INT(&store, pl->id);
        WRITE_STR(&store, pl->name);
        WRITE_INT(&store, pl->minx);
        WRITE_INT(&store, pl->maxx);
        WRITE_INT(&store, pl->miny);
        WRITE_INT(&store, pl->maxy);
        WRITE_INT(&store, pl->flags);
        w = pl->watchers;
        while (w) {
            if (w->faction) {
                write_faction_reference(w->faction, &store);
                WRITE_INT(&store, w->mode);
            }
            w = w->next;
        }
        write_faction_reference(NULL, &store);       /* mark the end of the list */
        a_write(&store, pl->attribs, pl);
        WRITE_SECTION(&store);
    }

    /* Write factions */
    write_alliances(&gdata);
    n = listlen(factions);
    WRITE_INT(&store, n);
    WRITE_SECTION(&store);

    log_debug(" - Schreibe %d Parteien...\n", n);
    for (f = factions; f; f = f->next) {
        if (fval(f, FFL_NPC)) {
            clear_npc_orders(f);
        }
        writefaction(&gdata, f);
        WRITE_SECTION(&store);
    }

    /* Write regions */

    n = listlen(regions);
    WRITE_INT(&store, n);
    WRITE_SECTION(&store);
    log_debug(" - Schreibe Regionen: %d", n);

    for (r = regions; r; r = r->next, --n) {
        /* plus leerzeile */
        if ((n % 1024) == 0) {      /* das spart extrem Zeit */
            log_debug(" - Schreibe Regionen: %d", n);
        }
        WRITE_SECTION(&store);
        WRITE_INT(&store, r->x);
        WRITE_INT(&store, r->y);
        writeregion(&gdata, r);

        WRITE_INT(&store, listlen(r->buildings));
        WRITE_SECTION(&store);
        for (b = r->buildings; b; b = b->next) {
            write_building_reference(b, &store);
            WRITE_STR(&store, b->name);
            WRITE_STR(&store, b->display ? b->display : "");
            WRITE_INT(&store, b->size);
            WRITE_TOK(&store, b->type->_name);
            WRITE_SECTION(&store);
            a_write(&store, b->attribs, b);
            WRITE_SECTION(&store);
        }

        WRITE_INT(&store, listlen(r->ships));
        WRITE_SECTION(&store);
        for (sh = r->ships; sh; sh = sh->next) {
            assert(sh->region == r);
            write_ship_reference(sh, &store);
            WRITE_STR(&store, (const char *)sh->name);
            WRITE_STR(&store, sh->display ? (const char *)sh->display : "");
            WRITE_TOK(&store, sh->type->_name);
            WRITE_INT(&store, sh->size);
            WRITE_INT(&store, sh->damage);
            WRITE_INT(&store, sh->flags & SFL_SAVEMASK);
            assert((sh->type->flags & SFL_NOCOAST) == 0 || sh->coast == NODIRECTION);
            WRITE_INT(&store, sh->coast);
            WRITE_SECTION(&store);
            a_write(&store, sh->attribs, sh);
            WRITE_SECTION(&store);
        }

        WRITE_INT(&store, listlen(r->units));
        WRITE_SECTION(&store);
        for (u = r->units; u; u = u->next) {
            write_unit(&gdata, u);
        }
    }
    WRITE_SECTION(&store);
    write_borders(&store);
    WRITE_SECTION(&store);

    binstore_done(&store);
    fstream_done(&strm);

    return 0;
}

void gamedata_close(gamedata *data) {
    binstore_done(data->store);
    fstream_done(&data->strm);
}

gamedata *gamedata_open(const char *filename, const char *mode) {
    FILE *F = fopen(filename, mode);

    if (F) {
        gamedata *data = (gamedata *)calloc(1, sizeof(gamedata));
        storage *store = (storage *)calloc(1, sizeof(storage));
        int err = 0;
        size_t sz;

        data->store = store;
        if (strchr(mode, 'r')) {
            sz = fread(&data->version, 1, sizeof(int), F);
            if (sz != sizeof(int)) {
                err = ferror(F);
            }
            else {
                err = fseek(F, sizeof(int), SEEK_CUR);
            }
        }
        else if (strchr(mode, 'w')) {
            int n = STREAM_VERSION;
            data->version = RELEASE_VERSION;
            fwrite(&data->version, sizeof(int), 1, F);
            fwrite(&n, sizeof(int), 1, F);
        }
        if (err) {
            fclose(F);
            free(data);
            free(store);
        }
        else {
            fstream_init(&data->strm, F);
            binstore_init(store, &data->strm);
            return data;
        }
    }
    log_error("could not open %s: %s", filename, strerror(errno));
    return 0;
}

int a_readint(attrib * a, void *owner, struct storage *store)
{
    /*  assert(sizeof(int)==sizeof(a->data)); */
    READ_INT(store, &a->data.i);
    return AT_READ_OK;
}

void a_writeint(const attrib * a, const void *owner, struct storage *store)
{
    WRITE_INT(store, a->data.i);
}

int a_readshorts(attrib * a, void *owner, struct storage *store)
{
    int n;
    READ_INT(store, &n);
    a->data.sa[0] = (short)n;
    READ_INT(store, &n);
    a->data.sa[1] = (short)n;
    return AT_READ_OK;
}

void a_writeshorts(const attrib * a, const void *owner, struct storage *store)
{
    WRITE_INT(store, a->data.sa[0]);
    WRITE_INT(store, a->data.sa[1]);
}

int a_readchars(attrib * a, void *owner, struct storage *store)
{
    int i;
    for (i = 0; i != 4; ++i) {
        int n;
        READ_INT(store, &n);
        a->data.ca[i] = (char)n;
    }
    return AT_READ_OK;
}

void a_writechars(const attrib * a, const void *owner, struct storage *store)
{
    int i;

    for (i = 0; i != 4; ++i) {
        WRITE_INT(store, a->data.ca[i]);
    }
}

int a_readvoid(attrib * a, void *owner, struct storage *store)
{
    return AT_READ_OK;
}

void a_writevoid(const attrib * a, const void *owner, struct storage *store)
{
}

int a_readstring(attrib * a, void *owner, struct storage *store)
{
    char buf[DISPLAYSIZE];
    char * result = 0;
    int e;
    size_t len = 0;
    do {
        e = READ_STR(store, buf, sizeof(buf));
        if (result) {
            result = realloc(result, len + DISPLAYSIZE - 1);
            strcpy(result + len, buf);
            len += DISPLAYSIZE - 1;
        }
        else {
            result = _strdup(buf);
        }
    } while (e == ENOMEM);
    a->data.v = result;
    return AT_READ_OK;
}

void a_writestring(const attrib * a, const void *owner, struct storage *store)
{
    assert(a->data.v);
    WRITE_STR(store, (const char *)a->data.v);
}

void a_finalizestring(attrib * a)
{
    free(a->data.v);
}
