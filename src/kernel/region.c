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
#include "region.h"

/* kernel includes */
#include "alliance.h"
#include "building.h"
#include "connection.h"
#include "curse.h"
#include "equipment.h"
#include "faction.h"
#include "item.h"
#include "messages.h"
#include "plane.h"
#include "region.h"
#include "resources.h"
#include "ship.h"
#include "terrain.h"
#include "terrainid.h"
#include "unit.h"

#include <spells/regioncurse.h>

/* util includes */
#include <util/assert.h>
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/gamedata.h>
#include <util/strings.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/umlaut.h>
#include <util/language.h>
#include <util/rand.h>
#include <util/rng.h>

#include <storage.h>

#include <modules/autoseed.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int dice_rand(const char *s);

region *regions;

int get_maxluxuries(void)
{
    const luxury_type *ltype;
    int maxluxuries = 0;
    for (ltype = luxurytypes; ltype; ltype = ltype->next) {
        ++maxluxuries;
    }
    return maxluxuries;
}

const int delta_x[MAXDIRECTIONS] = {
    -1, 0, 1, 1, 0, -1
};

const int delta_y[MAXDIRECTIONS] = {
    1, 1, 0, -1, -1, 0
};

static const direction_t back[MAXDIRECTIONS] = {
    D_SOUTHEAST,
    D_SOUTHWEST,
    D_WEST,
    D_NORTHWEST,
    D_NORTHEAST,
    D_EAST,
};

direction_t dir_invert(direction_t dir)
{
    switch (dir) {
    case D_PAUSE:
    case D_SPECIAL:
        return dir;
        break;
    default:
        if (dir >= 0 && dir < MAXDIRECTIONS)
            return back[dir];
    }
    assert(!"illegal direction");
    return NODIRECTION;
}

const char *write_regionname(const region * r, const faction * f, char *buffer,
    size_t size)
{
    char *buf = (char *)buffer;
    const struct locale *lang = f ? f->locale : 0;
    if (r == NULL) {
        strlcpy(buf, "(null)", size);
    }
    else {
        plane *pl = rplane(r);
        int nx = r->x, ny = r->y;
        pnormalize(&nx, &ny, pl);
        adjust_coordinates(f, &nx, &ny, pl);
        slprintf(buf, size, "%s (%d,%d)", rname(r, lang), nx, ny);
    }
    return buffer;
}

const char *regionname(const region * r, const faction * f)
{
    static int index = 0;
    static char buf[2][NAMESIZE];
    index = 1 - index;
    return write_regionname(r, f, buf[index], sizeof(buf[index]));
}

int region_maxworkers(const region *r)
{
    int size = production(r);
    int treespace = (rtrees(r, 2) + rtrees(r, 1) / 2) * TREESIZE;
    return MAX(size - treespace, MIN(size / 10, 200));
}

int deathcount(const region * r)
{
    attrib *a = a_find(r->attribs, &at_deathcount);
    if (!a)
        return 0;
    return a->data.i;
}

void deathcounts(region * r, int fallen)
{
    attrib *a = NULL;

    if (fallen == 0)
        return;
    if (r->attribs) {
        const curse_type *ctype = &ct_holyground;
        if (ctype && curse_active(get_curse(r->attribs, ctype)))
            return;
        a = a_find(r->attribs, &at_deathcount);
    }
    if (!a) {
        a = a_add(&r->attribs, a_new(&at_deathcount));
    }
    a->data.i += fallen;

    if (a->data.i <= 0) {
        a_remove(&r->attribs, a);
    }
}

/* Moveblock wird zur Zeit nicht über Attribute, sondern ein Bitfeld
   r->moveblock gemacht. Sollte umgestellt werden, wenn kompliziertere
   Dinge gefragt werden. */

/********************/
/*   at_moveblock   */
/********************/
void a_initmoveblock(attrib * a)
{
    a->data.v = calloc(1, sizeof(moveblock));
}

int a_readmoveblock(attrib * a, void *owner, gamedata *data)
{
    moveblock *m = (moveblock *)(a->data.v);
    int i;

    READ_INT(data->store, &i);
    m->dir = (direction_t)i;
    return AT_READ_OK;
}

void
a_writemoveblock(const attrib * a, const void *owner, struct storage *store)
{
    moveblock *m = (moveblock *)(a->data.v);
    WRITE_INT(store, (int)m->dir);
}

attrib_type at_moveblock = {
    "moveblock", a_initmoveblock, NULL, NULL, a_writemoveblock, a_readmoveblock
};

#define coor_hashkey(x, y) (unsigned int)((x<<16) + y)
#define RMAXHASH MAXREGIONS
static region *regionhash[RMAXHASH];
static int dummy_data;
static region *dummy_ptr = (region *)& dummy_data;     /* a funny hack */

typedef struct uidhashentry {
    int uid;
    region *r;
} uidhashentry;
static uidhashentry uidhash[MAXREGIONS];

struct region *findregionbyid(int uid)
{
    int key = uid % MAXREGIONS;
    while (uidhash[key].uid != 0 && uidhash[key].uid != uid) {
        if (++key == MAXREGIONS) key = 0;
    }
    return uidhash[key].r;
}

#define DELMARKER dummy_ptr

static void unhash_uid(region * r)
{
    int key = r->uid % MAXREGIONS;
    assert(r->uid);
    while (uidhash[key].uid != 0 && uidhash[key].uid != r->uid) {
        if (++key == MAXREGIONS) key = 0;
    }
    assert(uidhash[key].r == r);
    uidhash[key].r = NULL;
}

static void hash_uid(region * r)
{
    int uid = r->uid;
    for (;;) {
        if (uid != 0) {
            int key = uid % MAXREGIONS;
            while (uidhash[key].uid != 0 && uidhash[key].uid != uid) {
                if (++key == MAXREGIONS) key = 0;
            }
            if (uidhash[key].uid == 0) {
                uidhash[key].uid = uid;
                uidhash[key].r = r;
                break;
            }
            assert(uidhash[key].r != r || !"duplicate registration");
        }
        r->uid = uid = rng_int();
    }
}

#define HASH_STATISTICS 1
#if HASH_STATISTICS
static int hash_requests;
static int hash_misses;
#endif

bool pnormalize(int *x, int *y, const plane * pl)
{
    if (pl) {
        if (x) {
            int width = pl->maxx - pl->minx + 1;
            int nx = *x - pl->minx;
            nx = (nx > 0) ? nx : (width - (-nx) % width);
            *x = nx % width + pl->minx;
        }
        if (y) {
            int height = pl->maxy - pl->miny + 1;
            int ny = *y - pl->miny;
            ny = (ny > 0) ? ny : (height - (-ny) % height);
            *y = ny % height + pl->miny;
        }
    }
    return false;                 /* TBD */
}

static region *rfindhash(int x, int y)
{
    unsigned int rid = coor_hashkey(x, y);
    int key = HASH1(rid, RMAXHASH), gk = HASH2(rid, RMAXHASH);
#if HASH_STATISTICS
    ++hash_requests;
#endif
    while (regionhash[key] != NULL && (regionhash[key] == DELMARKER
        || regionhash[key]->x != x || regionhash[key]->y != y)) {
        key = (key + gk) % RMAXHASH;
#if HASH_STATISTICS
        ++hash_misses;
#endif
    }
    return regionhash[key];
}

void rhash(region * r)
{
    unsigned int rid = coor_hashkey(r->x, r->y);
    int key = HASH1(rid, RMAXHASH), gk = HASH2(rid, RMAXHASH);
    while (regionhash[key] != NULL && regionhash[key] != DELMARKER
        && regionhash[key] != r) {
        key = (key + gk) % RMAXHASH;
    }
    assert(regionhash[key] != r || !"trying to add the same region twice");
    regionhash[key] = r;
}

void runhash(region * r)
{
    unsigned int rid = coor_hashkey(r->x, r->y);
    int key = HASH1(rid, RMAXHASH), gk = HASH2(rid, RMAXHASH);

#ifdef FAST_CONNECT
    int d, di;
    for (d = 0, di = MAXDIRECTIONS / 2; d != MAXDIRECTIONS; ++d, ++di) {
        region *rc = r->connect[d];
        if (rc != NULL) {
            if (di >= MAXDIRECTIONS)
                di -= MAXDIRECTIONS;
            rc->connect[di] = NULL;
            r->connect[d] = NULL;
        }
    }
#endif
    while (regionhash[key] != NULL && regionhash[key] != r) {
        key = (key + gk) % RMAXHASH;
    }
    assert(regionhash[key] == r || !"trying to remove a unit that is not hashed");
    regionhash[key] = DELMARKER;
}

region *r_connect(const region * r, direction_t dir)
{
    region *result;
    int x, y;
#ifdef FAST_CONNECT
    region *rmodify = (region *)r;
    assert(dir >= 0 && dir < MAXDIRECTIONS);
    if (r->connect[dir])
        return r->connect[dir];
#endif
    assert(dir < MAXDIRECTIONS);
    x = r->x + delta_x[dir];
    y = r->y + delta_y[dir];
    pnormalize(&x, &y, rplane(r));
    result = rfindhash(x, y);
#ifdef FAST_CONNECT
    if (result) {
        rmodify->connect[dir] = result;
        result->connect[back[dir]] = rmodify;
    }
#endif
    return result;
}

region *findregion(int x, int y)
{
    return rfindhash(x, y);
}

/* Contributed by Hubert Mackenberg. Thanks.
 * x und y Abstand zwischen x1 und x2 berechnen
 */
static int koor_distance_orig(int x1, int y1, int x2, int y2)
{
    int dx = x1 - x2;
    int dy = y1 - y2;

    /* Bei negativem dy am Ursprung spiegeln, das veraendert
     * den Abstand nicht
     */
    if (dy < 0) {
        dy = -dy;
        dx = -dx;
    }

    /*
     * dy ist jetzt >=0, fuer dx sind 3 Faelle zu untescheiden
     */
    if (dx >= 0) {
        int result = dx + dy;
        return result;
    }
    else if (-dx >= dy) {
        int result = -dx;
        return result;
    }
    else {
        return dy;
    }
}

static int
koor_distance_wrap_xy(int x1, int y1, int x2, int y2, int width, int height)
{
    int dx = x1 - x2;
    int dy = y1 - y2;
    int result, dist;
    int mindist = MIN(width, height) >> 1;

    /* Bei negativem dy am Ursprung spiegeln, das veraendert
     * den Abstand nicht
     */
    if (dy < 0) {
        dy = -dy;
        dx = -dx;
    }
    if (dx < 0) {
        dx = width + dx;
    }
    /* dx,dy is now pointing northeast */
    result = dx + dy;
    if (result <= mindist)
        return result;

    dist = (width - dx) + (height - dy);  /* southwest */
    if (dist >= 0 && dist < result) {
        result = dist;
        if (result <= mindist)
            return result;
    }
    dist = MAX(dx, height - dy);
    if (dist >= 0 && dist < result) {
        result = dist;
        if (result <= mindist)
            return result;
    }
    dist = MAX(width - dx, dy);
    if (dist >= 0 && dist < result)
        result = dist;
    return result;
}

int koor_distance(int x1, int y1, int x2, int y2)
{
    const plane *p1 = findplane(x1, y1);
    const plane *p2 = findplane(x2, y2);
    if (p1 != p2)
        return INT_MAX;
    else {
        int width = plane_width(p1);
        int height = plane_height(p1);
        if (width && height) {
            return koor_distance_wrap_xy(x1, y1, x2, y2, width, height);
        }
        else {
            return koor_distance_orig(x1, y1, x2, y2);
        }
    }
}

int distance(const region * r1, const region * r2)
{
    return koor_distance(r1->x, r1->y, r2->x, r2->y);
}

void free_regionlist(region_list * rl)
{
    while (rl) {
        region_list *rl2 = rl->next;
        free(rl);
        rl = rl2;
    }
}

void add_regionlist(region_list ** rl, region * r)
{
    region_list *rl2 = (region_list *)malloc(sizeof(region_list));

    rl2->data = r;
    rl2->next = *rl;

    *rl = rl2;
}

/********************/
/*   at_horseluck   */
/********************/
attrib_type at_horseluck = {
    "horseluck",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ,
    NULL,
    ATF_UNIQUE
};

/**********************/
/*   at_peasantluck   */
/**********************/
attrib_type at_peasantluck = {
    "peasantluck",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ,
    NULL,
    ATF_UNIQUE
};

/*********************/
/*   at_deathcount   */
/*********************/
attrib_type at_deathcount = {
    "deathcount",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    a_writeint,
    a_readint,
    NULL,
    ATF_UNIQUE
};

/*********************/
/*   at_woodcount   */
/*********************/
attrib_type at_woodcount = {
    "woodcount",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    a_readint,
    NULL,
    ATF_UNIQUE
};

void rsetroad(region * r, direction_t d, int val)
{
    connection *b;
    region *r2 = rconnect(r, d);

    assert(val>=SHRT_MIN && val<=SHRT_MAX);
    if (!r2) {
        return;
    }
    b = get_borders(r, r2);
    while (b && b->type != &bt_road) {
        b = b->next;
    }
    if (!b) {
        if (!val) return;
        b = new_border(&bt_road, r, r2);
    }
    if (r == b->from) {
        b->data.sa[0] = (short)val;
    }
    else {
        b->data.sa[1] = (short)val;
    }
}

int rroad(const region * r, direction_t d)
{
    connection *b;
    region *r2 = rconnect(r, d);

    if (!r2) {
        return 0;
    }
    b = get_borders(r, r2);
    while (b && b->type != &bt_road) {
        b = b->next;
    }
    if (!b) {
        return 0;
    }

    return (r == b->from) ? b->data.sa[0] : b->data.sa[1];
}

bool r_isforest(const region * r)
{
    if (fval(r->terrain, FOREST_REGION)) {
        /* needs to be covered with at least 48% trees */
        int mincover = (int)(r->terrain->size * 0.48);
        int trees = rtrees(r, 2) + rtrees(r, 1);
        return (trees * TREESIZE >= mincover);
    }
    return false;
}

bool is_coastregion(region * r)
{
    direction_t i;
    int res = 0;

    for (i = 0; !res && i < MAXDIRECTIONS; i++) {
        region *rn = rconnect(r, i);
        if (rn && fval(rn->terrain, SEA_REGION))
            res++;
    }
    return res != 0;
}

int rpeasants(const region * r)
{
    int value = 0;
    if (r->land) {
        value = r->land->peasants;
        assert(value >= 0);
    }
    return value;
}

void rsetpeasants(region * r, int value)
{
    assert(r->land || value==0);
    assert(value >= 0);
    if (r->land) {
        r->land->peasants = value;
    }
}

int rmoney(const region * r)
{
    return r->land ? r->land->money : 0;
}

void rsethorses(const region * r, int value)
{
    assert(r->land || value==0);
    assert(value >= 0);
    if (r->land) {
        r->land->horses = value;
    }
}

int rhorses(const region * r)
{
    return r->land ? r->land->horses : 0;
}

void rsetmoney(region * r, int value)
{
    assert(r && (r->land || value==0));
    assert(value >= 0);
    if (r->land) {
        r->land->money = value;
    }
}

int rherbs(const region *r)
{
    return r->land?r->land->herbs:0;
}

void rsetherbs(region *r, int value)
{
    assert(r->land || value==0);
    assert(value >= 0 && value<=SHRT_MAX);
    if (r->land) {
        r->land->herbs = (short)value;
    }
}

void rsetherbtype(region *r, const struct item_type *itype) {
    assert(r->land && r->terrain);
    if (itype == NULL) {
        r->land->herbtype = NULL;
    }
    else {
        if (r->terrain->herbs) {
            int i;
            for (i = 0; r->terrain->herbs[i]; ++i) {
                if (r->terrain->herbs[i] == itype) {
                    r->land->herbtype = itype;
                    return;
                }
            }
        }
        log_debug("attempt to set herbtype=%s for terrain=%s in %s", itype->rtype->_name, r->terrain->_name, regionname(r, 0));
        r->land->herbtype = itype;
    }
}

void r_setdemand(region * r, const luxury_type * ltype, int value)
{
    struct demand *d, **dp = &r->land->demands;

    if (ltype == NULL)
        return;

    while (*dp && (*dp)->type != ltype)
        dp = &(*dp)->next;
    d = *dp;
    if (!d) {
        d = *dp = malloc(sizeof(struct demand));
        assert_alloc(d);
        d->next = NULL;
        d->type = ltype;
    }
    d->value = value;
}

const item_type *r_luxury(const region * r)
{
    struct demand *dmd;
    if (r->land) {
        assert(r->land->demands || !"need to call fix_demands on a region");
        for (dmd = r->land->demands; dmd; dmd = dmd->next) {
            if (dmd->value == 0)
                return dmd->type->itype;
        }
    }
    return NULL;
}

int r_demand(const region * r, const luxury_type * ltype)
{
    struct demand *d = r->land->demands;
    while (d && d->type != ltype)
        d = d->next;
    if (!d)
        return -1;
    return d->value;
}

const char *rname(const region * r, const struct locale *lang)
{
    if (r->land && r->land->name) {
        return r->land->name;
    }
    return LOC(lang, terrain_name(r));
}

int rtrees(const region * r, int ageclass)
{
    return ((r)->land ? (r)->land->trees[ageclass] : 0);
}

int rsettrees(const region * r, int ageclass, int value)
{
    if (!r->land) {
        assert(value == 0);
    }
    else {
        assert(value >= 0);
        if (value <= MAXTREES) {
            return r->land->trees[ageclass] = value;
        }
        else {
            r->land->trees[ageclass] = MAXTREES;
        }
    }
    return 0;
}

static region *last;

static unsigned int max_index = 0;

region *new_region(int x, int y, struct plane *pl, int uid)
{
    region *r;

    pnormalize(&x, &y, pl);
    r = rfindhash(x, y);

    if (r) {
        log_error("duplicate region discovered: %s(%d,%d)\n", regionname(r, NULL), x, y);
        if (r->units)
            log_error("duplicate region contains units\n");
        return r;
    }
    r = calloc(1, sizeof(region));
    assert_alloc(r);
    r->x = x;
    r->y = y;
    r->uid = uid;
    r->age = 1;
    r->_plane = pl;
    rhash(r);
    hash_uid(r);
    if (last)
        addlist(&last, r);
    else
        addlist(&regions, r);
    last = r;
    assert(r->next == NULL);
    r->index = ++max_index;
    return r;
}

static region *deleted_regions;

void remove_region(region ** rlist, region * r)
{

    while (r->units) {
        unit *u = r->units;
        i_freeall(&u->items);
        remove_unit(&r->units, u);
    }

    runhash(r);
    unhash_uid(r);
    while (*rlist && *rlist != r)
        rlist = &(*rlist)->next;
    assert(*rlist == r);
    *rlist = r->next;
    r->next = deleted_regions;
    deleted_regions = r;
}

void free_land(land_region * lr)
{
    free(lr->ownership);
    while (lr->demands) {
        struct demand *d = lr->demands;
        lr->demands = d->next;
        free(d);
    }
    free(lr->name);
    free(lr->display);
    free(lr);
}

void region_setresource(region * r, const resource_type * rtype, int value)
{
    rawmaterial *rm = r->resources;
    while (rm) {
        if (rm->rtype == rtype) {
            rm->amount = value;
            break;
        }
        rm = rm->next;
    }
    if (!rm) {
        if (rtype == get_resourcetype(R_SILVER))
            rsetmoney(r, value);
        else if (rtype == get_resourcetype(R_PEASANT))
            rsetpeasants(r, value);
        else if (rtype == get_resourcetype(R_HORSE))
            rsethorses(r, value);
        else {
            rawmaterial *rm;
            if (r->terrain->production) {
                int i;
                for (i = 0; r->terrain->production[i].type; ++i) {
                    const terrain_production *production = r->terrain->production + i;
                    if (production->type == rtype) {
                        add_resource(r, 1, value, dice_rand(production->divisor), rtype);
                        return;
                    }
                }
            }
            /* adamantium etc are not usually terraformed: */
            for (rm = r->resources; rm; rm = rm->next) {
                if (rm->rtype == rtype) {
                    rm->amount = value;
                    return;
                }
            }
            if (!rm) {
                add_resource(r, 1, value, 150, rtype);
            }
        }
    }
}

int region_getresource(const region * r, const resource_type * rtype)
{
    const rawmaterial *rm;
    for (rm = r->resources; rm; rm = rm->next) {
        if (rm->rtype == rtype) {
            return rm->amount;
        }
    }
    if (rtype == get_resourcetype(R_SILVER))
        return rmoney(r);
    if (rtype == get_resourcetype(R_HORSE))
        return rhorses(r);
    if (rtype == get_resourcetype(R_PEASANT))
        return rpeasants(r);
    return 0;
}

void free_region(region * r)
{
    if (last == r)
        last = NULL;
    if (r->land)
        free_land(r->land);

    if (r->msgs) {
        free_messagelist(r->msgs->begin);
        free(r->msgs);
        r->msgs = 0;
    }

    while (r->individual_messages) {
        struct individual_message *msg = r->individual_messages;
        r->individual_messages = msg->next;
        if (msg->msgs) {
            free_messagelist(msg->msgs->begin);
            free(msg->msgs);
        }
        free(msg);
    }

    while (r->attribs)
        a_remove(&r->attribs, r->attribs);
    while (r->resources) {
        rawmaterial *res = r->resources;
        r->resources = res->next;
        free(res);
    }

    while (r->units) {
        unit *u = r->units;
        r->units = u->next;
        u->region = NULL;
        uunhash(u);
        free_unit(u);
        free(u);
    }

    while (r->buildings) {
        building *b = r->buildings;
        assert(b->region == r);
        r->buildings = b->next;
        bunhash(b);                 /* must be done here, because remove_building does it, and wasn't called */
        free_building(b);
    }

    while (r->ships) {
        ship *s = r->ships;
        assert(s->region == r);
        r->ships = s->next;
        sunhash(s);
        free_ship(s);
    }

    free(r);
}

void free_regions(void)
{
    memset(uidhash, 0, sizeof(uidhash));
    while (deleted_regions) {
        region *r = deleted_regions;
        deleted_regions = r->next;
        free_region(r);
    }
    while (regions) {
        region *r = regions;
        regions = r->next;
        runhash(r);
        free_region(r);
    }
    max_index = 0;
    last = NULL;
}

/** creates a name for a region
 * TODO: Make vowels XML-configurable and allow non-ascii characters again.
 * - that will probably require a wchar_t * string to pick from.
 */
static char *makename(void)
{
    int s, v, k, e, p = 0, x = 0;
    size_t nk, ne, nv, ns;
    static char name[16];
    const char *kons = "bcdfghklmnprstvwz",
        *start = "bcdgtskpvfr",
        *end = "nlrdst",
        *vowels = "aaaaaaaaaaaeeeeeeeeeeeeiiiiiiiiiiioooooooooooouuuuuuuuuuyy";

    /* const char * vowels_latin1 = "aaaaaaaaaàâeeeeeeeeeéèêiiiiiiiiiíîoooooooooóòôuuuuuuuuuúyy"; */

    nk = strlen(kons);
    ne = strlen(end);
    nv = strlen(vowels);
    ns = strlen(start);

    for (s = rng_int() % 3 + 2; s > 0; s--) {
        if (x > 0) {
            k = rng_int() % (int)nk;
            name[p] = kons[k];
            p++;
        }
        else {
            k = rng_int() % (int)ns;
            name[p] = start[k];
            p++;
        }
        v = rng_int() % (int)nv;
        name[p] = vowels[v];
        p++;
        if (rng_int() % 3 == 2 || s == 1) {
            e = rng_int() % (int)ne;
            name[p] = end[e];
            p++;
            x = 1;
        }
        else
            x = 0;
    }
    name[p] = '\0';
    name[0] = (char)toupper(name[0]);
    return name;
}

void setluxuries(region * r, const luxury_type * sale)
{
    const luxury_type *ltype;

    assert(r->land);

    if (r->land->demands) {
        freelist(r->land->demands);
        r->land->demands = 0;
    }

    for (ltype = luxurytypes; ltype; ltype = ltype->next) {
        struct demand *dmd = malloc(sizeof(struct demand));
        dmd->type = ltype;
        if (ltype != sale)
            dmd->value = 1 + rng_int() % 5;
        else
            dmd->value = 0;
        dmd->next = r->land->demands;
        r->land->demands = dmd;
    }
}

int fix_demand(region * rd) {
    luxury_type * ltype;
    int maxluxuries = get_maxluxuries();
    if (maxluxuries > 0) {
        int sale = rng_int() % maxluxuries;
        for (ltype = luxurytypes; sale != 0 && ltype; ltype = ltype->next) {
            --sale;
        }
        setluxuries(rd, ltype);
        return 0;
    }
    return -1;
}

void terraform_region(region * r, const terrain_type * terrain)
{
    /* Resourcen, die nicht mehr vorkommen können, löschen */
    const terrain_type *oldterrain = r->terrain;
    rawmaterial **lrm = &r->resources;

    assert(terrain);

    while (*lrm) {
        rawmaterial *rm = *lrm;
        const resource_type *rtype = NULL;

        if (terrain->production != NULL) {
            int i;
            for (i = 0; terrain->production[i].type; ++i) {
                if (rm->rtype == terrain->production[i].type) {
                    rtype = rm->rtype;
                    break;
                }
            }
        }
        if (rtype == NULL) {
            *lrm = rm->next;
            free(rm);
        }
        else {
            lrm = &rm->next;
        }
    }

    r->terrain = terrain;
    terraform_resources(r);

    if (!fval(terrain, LAND_REGION)) {
        if (r->land) {
            free_land(r->land);
            r->land = NULL;
        }
        rsettrees(r, 0, 0);
        rsettrees(r, 1, 0);
        rsettrees(r, 2, 0);
        rsethorses(r, 0);
        rsetpeasants(r, 0);
        rsetmoney(r, 0);
        freset(r, RF_ENCOUNTER);
        freset(r, RF_MALLORN);
        return;
    }

    if (r->land) {
        int d;
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            rsetroad(r, d, 0);
        }
    }
    else {
        static struct surround {
            struct surround *next;
            const luxury_type *type;
            int value;
        } *trash = NULL, *nb = NULL;
        const luxury_type *ltype = NULL;
        direction_t d;
        int mnr = 0;

        r->land = calloc(1, sizeof(land_region));
        r->land->ownership = NULL;
        region_set_morale(r, MORALE_DEFAULT, -1);
        region_setname(r, makename());
        fix_demand(r);
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *nr = rconnect(r, d);
            if (nr && nr->land) {
                struct demand *sale = r->land->demands;
                while (sale && sale->value != 0)
                    sale = sale->next;
                if (sale) {
                    struct surround *sr = nb;
                    while (sr && sr->type != sale->type)
                        sr = sr->next;
                    if (!sr) {
                        if (trash) {
                            sr = trash;
                            trash = trash->next;
                        }
                        else {
                            sr = calloc(1, sizeof(struct surround));
                        }
                        sr->next = nb;
                        sr->type = sale->type;
                        sr->value = 1;
                        nb = sr;
                    }
                    else
                        sr->value++;
                    ++mnr;
                }
            }
        }
        if (!nb) {
            /* TODO: this is really lame */
            int i = get_maxluxuries();
            if (i > 0) {
                i = rng_int() % i;
                ltype = luxurytypes;
                while (i--)
                    ltype = ltype->next;
            }
        }
        else {
            int i = rng_int() % mnr;
            struct surround *srd = nb;
            while (i > srd->value) {
                i -= srd->value;
                srd = srd->next;
            }
            if (srd->type)
                setluxuries(r, srd->type);
            while (srd->next != NULL)
                srd = srd->next;
            srd->next = trash;
            trash = nb;
            nb = NULL;
        }
    }

    if (fval(terrain, LAND_REGION)) {
        const item_type *itype = NULL;

        if (r->terrain->herbs) {
            int len = 0;
            while (r->terrain->herbs[len])
                ++len;
            if (len)
                itype = r->terrain->herbs[rng_int() % len];
        }
        if (itype != NULL) {
            rsetherbtype(r, itype);
            rsetherbs(r, (short)(50 + rng_int() % 31));
        }
        else {
            rsetherbtype(r, NULL);
        }
        if (oldterrain == NULL || !fval(oldterrain, LAND_REGION)) {
            if (rng_int() % 100 < 3)
                fset(r, RF_MALLORN);
            else
                freset(r, RF_MALLORN);
            if (rng_int() % 100 < ENCCHANCE) {
                fset(r, RF_ENCOUNTER);
            }
        }
    }

    if (oldterrain == NULL || terrain->size != oldterrain->size) {
        int horses = 0, trees = 0;
        if (terrain->size>0) {
            horses = rng_int() % (terrain->size / 50);
            trees = terrain->size * (30 + rng_int() % 40) / 1000;
        }
        if (terrain == newterrain(T_PLAIN)) {
            rsethorses(r, horses);
            if (chance(0.4)) {
                rsettrees(r, 2, trees);
            }
        }
        else if (trees>0 && chance(0.2)) {
            rsettrees(r, 2, trees);
        }
        else {
            rsettrees(r, 2, 0);
        }
        rsettrees(r, 1, rtrees(r, 2) / 4);
        rsettrees(r, 0, rtrees(r, 2) / 8);

        if (!fval(r, RF_CHAOTIC)) {
            int peasants;
            peasants = (region_maxworkers(r) * (20 + dice(6, 10))) / 100;
            rsetpeasants(r, MAX(100, peasants));
            rsetmoney(r, rpeasants(r) * ((wage(r, NULL, NULL,
                INT_MAX) + 1) + rng_int() % 5));
        }
    }
}

/** ENNO:
 * ich denke, das das hier nicht sein sollte.
 * statt dessen sollte ein attribut an der region sein, dass das erledigt,
 * egal ob durch den spell oder anderes angelegt.
 **/
#include "curse.h"
int production(const region * r)
{
    /* muß rterrain(r) sein, nicht rterrain() wegen rekursion */
    int p = r->terrain->size;
    if (curse_active(get_curse(r->attribs, &ct_drought))) {
        p /= 2;
    }

    return p;
}

void resolve_region(region *r)
{
    resolve(RESOLVE_REGION | r->uid, r);
}

int read_region_reference(gamedata * data, region **rp, resolve_fun fun)
{
    struct storage * store = data->store;
    int id = 0;

    READ_INT(store, &id);
    *rp = findregionbyid(id);
    if (*rp == NULL) {
        ur_add(RESOLVE_REGION | id, (void **)rp, fun);
    }
    return id;
}

void write_region_reference(const region * r, struct storage *store)
{
    if (r) {
        WRITE_INT(store, r->uid);
    }
    else {
        WRITE_INT(store, 0);
    }
}

struct message_list *r_getmessages(const struct region *r,
    const struct faction *viewer)
{
    struct individual_message *imsg = r->individual_messages;
    while (imsg && (imsg)->viewer != viewer)
        imsg = imsg->next;
    if (imsg)
        return imsg->msgs;
    return NULL;
}

struct message *r_addmessage(struct region *r, const struct faction *viewer,
struct message *msg)
{
    assert(r);
    if (viewer) {
        struct individual_message *imsg;
        imsg = r->individual_messages;
        while (imsg && imsg->viewer != viewer)
            imsg = imsg->next;
        if (imsg == NULL) {
            imsg = malloc(sizeof(struct individual_message));
            imsg->next = r->individual_messages;
            imsg->msgs = NULL;
            r->individual_messages = imsg;
            imsg->viewer = viewer;
        }
        return add_message(&imsg->msgs, msg);
    }
    return add_message(&r->msgs, msg);
}

struct faction *region_get_owner(const struct region *r)
{
    if (r->land) {
        if (rule_region_owners()) {
            if (r->land->ownership) {
                return r->land->ownership->owner;
            }
        }
        else {
            building *b = largestbuilding(r, cmp_castle_size, false);
            unit * u = b ? building_owner(b) : NULL;
            return u ? u->faction : NULL;
        }
    }
    return NULL;
}

struct faction *region_get_last_owner(const struct region *r)
{
    assert(rule_region_owners());
    if (r->land && r->land->ownership) {
        return r->land->ownership->last_owner;
    }
    return NULL;
}

struct alliance *region_get_alliance(const struct region *r)
{
    assert(rule_region_owners());
    if (r->land && r->land->ownership) {
        region_owner *own = r->land->ownership;
        return own->owner ? own->owner->alliance : (own->last_owner? own->last_owner->alliance : NULL);
    }
    return NULL;
}

void region_set_owner(struct region *r, struct faction *owner, int turn)
{
    assert(rule_region_owners());
    if (r->land) {
        if (!r->land->ownership) {
            r->land->ownership = malloc(sizeof(region_owner));
            assert(region_get_morale(r) == MORALE_DEFAULT);
            r->land->ownership->owner = NULL;
            r->land->ownership->last_owner = NULL;
            r->land->ownership->flags = 0;
        }
        r->land->ownership->since_turn = turn;
        r->land->ownership->morale_turn = turn;
        assert(r->land->ownership->owner != owner);
        r->land->ownership->last_owner = r->land->ownership->owner;
        r->land->ownership->owner = owner;
    }
}

faction *update_owners(region * r)
{
    faction *f = NULL;
    assert(rule_region_owners());
    if (r->land) {
        building *bowner = largestbuilding(r, cmp_current_owner, false);
        building *blargest = largestbuilding(r, cmp_taxes, false);
        if (blargest) {
            if (!bowner || bowner->size < blargest->size) {
                /* region owners update? */
                unit *new_owner = building_owner(blargest);
                f = region_get_owner(r);
                if (new_owner == NULL) {
                    if (f) {
                        region_set_owner(r, NULL, turn);
                        f = NULL;
                    }
                }
                else if (new_owner->faction != f) {
                    if (!r->land->ownership) {
                        /* there has never been a prior owner */
                        region_set_morale(r, MORALE_DEFAULT, turn);
                    }
                    else if (f || new_owner->faction != region_get_last_owner(r)) {
                        alliance *al = region_get_alliance(r);
                        if (al && new_owner->faction->alliance == al) {
                            int morale = MAX(0, region_get_morale(r) - MORALE_TRANSFER);
                            region_set_morale(r, morale, turn);
                        }
                        else {
                            region_set_morale(r, MORALE_TAKEOVER, turn);
                        }
                    }
                    region_set_owner(r, new_owner->faction, turn);
                    f = new_owner->faction;
                }
            }
        }
        else if (r->land->ownership && r->land->ownership->owner) {
            region_set_owner(r, NULL, turn);
            f = NULL;
        }
    }
    return f;
}

void region_setinfo(struct region *r, const char *info)
{
    assert(r->land);
    free(r->land->display);
    r->land->display = (info && info[0]) ? strdup(info) : 0;
}

const char *region_getinfo(const region * r)
{
    return (r->land && r->land->display) ? r->land->display : "";
}

void region_setname(struct region *r, const char *name)
{
    if (r->land) {
        free(r->land->name);
        r->land->name = name ? strdup(name) : 0;
    }
}

const char *region_getname(const region * r)
{
    if (r->land && r->land->name) {
        return r->land->name;
    }
    return "";
}

int region_get_morale(const region * r)
{
    if (r->land) {
        assert(r->land->morale >= 0 && r->land->morale <= MORALE_MAX);
        return r->land->morale;
    }
    return -1;
}

void region_set_morale(region * r, int morale, int turn)
{
    if (r->land) {
        r->land->morale = (short)morale;
        if (turn >= 0 && r->land->ownership) {
            r->land->ownership->morale_turn = turn;
        }
        assert(r->land->morale >= 0 && r->land->morale <= MORALE_MAX);
    }
}

void get_neighbours(const region * r, region ** list)
{
    int dir;
    for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
        list[dir] = rconnect(r, (direction_t)dir);
    }
}

int owner_change(const region * r)
{
    if (r->land && r->land->ownership) {
        return r->land->ownership->since_turn;
    }
    return INT_MIN;
}

bool is_mourning(const region * r, int in_turn)
{
    int change = owner_change(r);
    return (change == in_turn - 1 && r->land &&
        r->land->ownership->last_owner && r->land->ownership->owner &&
        r->land->ownership->last_owner != r->land->ownership->owner);
}
