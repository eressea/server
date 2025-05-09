#include "region.h"

/* kernel includes */
#include "alliance.h"
#include "attrib.h"
#include "building.h"
#include "calendar.h"
#include "config.h"
#include "connection.h"
#include "curse.h"
#include "equipment.h"
#include "faction.h"
#include "gamedata.h"
#include "item.h"
#include "messages.h"
#include "plane.h"
#include "resources.h"
#include "ship.h"
#include "teleport.h"
#include "terrain.h"
#include "terrainid.h"
#include "unit.h"

#include <spells/regioncurse.h>

/* util includes */
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/umlaut.h>

#include <modules/autoseed.h>

#include <storage.h>
#include <strings.h>

#include <stb_ds.h>

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
        str_strlcpy(buf, "(null)", size);
    }
    else {
        plane *pl = rplane(r);
        int nx = r->x, ny = r->y;
        pnormalize(&nx, &ny, pl);
        adjust_coordinates(f, &nx, &ny, pl);
        snprintf(buf, size, "%s (%d,%d)", rname(r, lang), nx, ny);
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

int region_maxworkers(const region* r, int size)
{
    return size - (rtrees(r, 2) + rtrees(r, 1) / 2) * TREESIZE;
}

int region_production(const region* r)
{
    int size = max_production(r);
    int treespace = region_maxworkers(r, size);
    size /=10;
    if (size > 200) size = 200;
    if (treespace < size) treespace = size;
    return treespace;
}

int deathcount(const region * r)
{
    if (r->land) {
        attrib *a = a_find(r->attribs, &at_deathcount);
        if (a) {
            return a->data.i;
        }
    }
    return 0;
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

/* Moveblock wird zur Zeit nicht ueber Attribute, sondern ein Bitfeld
   r->moveblock gemacht. Sollte umgestellt werden, wenn kompliziertere
   Dinge gefragt werden. */

/********************/
/*   at_moveblock   */
/********************/
void a_initmoveblock(variant *var)
{
    var->v = calloc(1, sizeof(moveblock));
}

int a_readmoveblock(variant *var, void *owner, gamedata *data)
{
    moveblock *m = (moveblock *)var->v;
    int i;

    READ_INT(data->store, &i);
    m->dir = (direction_t)i;
    return AT_READ_OK;
}

void
a_writemoveblock(const variant *var, const void *owner, struct storage *store)
{
    moveblock *m = (moveblock *)var->v;
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

static void rhash_uid(region * r)
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
        r->uid = uid = genrand_int31();
    }
}

#define HASH_STATISTICS 1
#if HASH_STATISTICS
static int hash_requests;
static int hash_misses;
#endif

void pnormalize(int *x, int *y, const plane * pl)
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

void region_set_uid(region *r, int uid)
{
    unhash_uid(r);
    r->uid = uid;
    rhash_uid(r);
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
    while (regionhash[key] != NULL && regionhash[key] != r) {
        key = (key + gk) % RMAXHASH;
    }
    assert(regionhash[key] == r || !"trying to remove a unit that is not hashed");
    regionhash[key] = DELMARKER;
}

region *rconnect(const region * r, direction_t dir)
{
    region *result;
    int x, y;
    if (dir < 0 || dir >= MAXDIRECTIONS) {
        return NULL;
    }
    if (r->connect[dir]) {
        return r->connect[dir];
    }
    x = r->x + delta_x[dir];
    y = r->y + delta_y[dir];
    pnormalize(&x, &y, rplane(r));
    result = rfindhash(x, y);
    if (result) {
        region *rmodify = (region *)r;
        rmodify->connect[dir] = result;
        result->connect[back[dir]] = rmodify;
    }
    return result;
}

struct region *findregion_ex(int x, int y, const struct plane *pl)
{
    if (pl) {
        pnormalize(&x, &y, pl);
    }
    return rfindhash(x, y);
}

struct region *findregion(int x, int y)
{
    return findregion_ex(x, y, NULL);
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
    int mindist = ((width > height) ? height : width) / 2;

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
    dist = height - dy;
    if (dist < dx) dist = dx;
    if (dist >= 0 && dist < result) {
        result = dist;
        if (result <= mindist)
            return result;
    }
    dist = width - dx;
    if (dist < dy) dist = dy;
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

    if (!rl2) abort();
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

void rsetroad(region * r, direction_t d, short val)
{
    if (!r->land) return;
    r->land->roads[d] = val;
}

short rroad(const region * r, direction_t d)
{
    return r->land ? r->land->roads[d] : 0;
}

void r_foreach_demand(const struct region *r, void (*callback)(struct demand *, int, void *), void *data)
{
    assert(r);
    if (r->land && r->land->_demands) {
        size_t n = arrlen(r->land->_demands);
        if (n > 0 && n <= INT_MAX) {
            callback(r->land->_demands, (int)n, data);
        }
    }
}

bool r_has_demands(const struct region *r)
{
    return r->land && r->land->_demands;
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

int rsetpeasants(region * r, int value)
{
    assert(r->land || value==0);
    assert(value >= 0);
    if (r->land) {
        if (value > USHRT_MAX) {
            log_warning("region %s cannot have %d peasants.", regionname(r, NULL), value);
            value = USHRT_MAX;
        }
        r->land->peasants = (unsigned short)value;
        return r->land->peasants;
    }
    return 0;
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
        if (value > USHRT_MAX) {
            log_warning("region %s cannot have %d horses.", regionname(r, NULL), value);
            value = USHRT_MAX;
        }
        r->land->horses = (unsigned short)value;
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
    return r->land ? r->land->herbs : 0;
}

void rsetherbs(region *r, int value)
{
    assert(r->land || value==0);
    if (r->land) {
        if (value > USHRT_MAX) {
            log_warning("region %s cannot have %d herbs.", regionname(r, NULL), value);
            value = USHRT_MAX;
        }
        r->land->herbs = (unsigned short)value;
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
            if (i > 0) {
                i = rng_int() % i;
            }
            r->land->herbtype = r->terrain->herbs[i];
        }
        else {
            r->land->herbtype = itype;
        }
        log_warning("attempted to set herbtype=%s for terrain=%s in %s", itype->rtype->_name, r->terrain->_name, regionname(r, 0));
    }
}

void r_setdemand(region * r, const luxury_type * ltype, int value)
{
    ptrdiff_t i;
    struct demand *d = NULL;

    if (ltype == NULL)
        return;

    for (i = arrlen(r->land->_demands) - 1; i >= 0; --i) {
        if (r->land->_demands[i].type == ltype) {
            d = r->land->_demands + i;
            break;
        }
    }

    if (!d) {
        d = arraddnptr(r->land->_demands, 1);
        d->type = ltype;
    }
    d->value = value;
}

const item_type *r_luxury(const region * r)
{
    assert(r);
    if (r->land && r->land->_demands) {
        ptrdiff_t i;
        for (i = arrlen(r->land->_demands) - 1; i >= 0; --i) {
            const struct demand *dmd = r->land->_demands + i;
            if (dmd->value == 0) {
                return dmd->type->itype;
            }
        }
    }
    return NULL;
}

int r_demand(const region * r, const luxury_type * ltype)
{
    assert(r);
    if (r->land && r->land->_demands) {
        ptrdiff_t i;
        for (i = arrlen(r->land->_demands) - 1; i >= 0; --i) {
            const struct demand *dmd = r->land->_demands + i;
            if (dmd->type == ltype) {
                return dmd->value;
            }
        }
    }
    return -1;
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
        return 0;
    }
    assert(value >= 0);
    if (value < MAXTREES) {
        r->land->trees[ageclass] = value;
    }
    else {
        r->land->trees[ageclass] = MAXTREES;
    }
    return r->land->trees[ageclass];
}

region *region_create(int uid)
{
    region *r = (region *)calloc(1, sizeof(region));
    assert(r);
    r->uid = uid;
    rhash_uid(r);
    return r;
}

static region *last;
static unsigned int max_index;

void add_region(region *r, int x, int y) {
    r->x = x;
    r->y = y;
    r->_plane = findplane(x, y);
    rhash(r);
    if (last) {
        addlist(&last, r);
    }
    else {
        addlist(&regions, r);
    }
    last = r;
    assert(r->next == NULL);
    r->index = ++max_index;
}

region *new_region(int x, int y, struct plane *pl, int uid)
{
    region *r;
    r = region_create(uid);
    r->age = 0;
    add_region(r, x, y);
    return r;
}

static region *deleted_regions;

void remove_region(region ** rlist, region * r)
{
    assert(r);
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
    arrfree(lr->_demands);
    free(lr->name);
    free(lr->display);
    free(lr);
}

struct rawmaterial *region_setresource(region * r, const struct resource_type *rtype, int value)
{
    rawmaterial *rm = rm_get(r, rtype);
    if (rm) {
        rm->amount = value;
    }
    else {
        if (rtype == get_resourcetype(R_SILVER))
            rsetmoney(r, value);
        else if (rtype == get_resourcetype(R_PEASANT))
            rsetpeasants(r, value);
        else if (rtype == get_resourcetype(R_HORSE))
            rsethorses(r, value);
        else if (rtype == get_resourcetype(R_TREE)) {
            rsettrees(r, TREE_TREE, value);
            freset(r, RF_MALLORN);
        }
        else if (rtype == get_resourcetype(R_SAPLING)) {
            rsettrees(r, TREE_SAPLING, value);
            freset(r, RF_MALLORN);
        }
        else if (rtype == get_resourcetype(R_SEED)) {
            rsettrees(r, TREE_SEED, value);
            freset(r, RF_MALLORN);
        }
        else if (rtype == get_resourcetype(R_MALLORN_TREE)) {
            rsettrees(r, TREE_TREE, value);
            fset(r, RF_MALLORN);
        }
        else if (rtype == get_resourcetype(R_MALLORN_SAPLING)) {
            rsettrees(r, TREE_SAPLING, value);
            fset(r, RF_MALLORN);
        }
        else if (rtype == get_resourcetype(R_MALLORN_SEED)) {
            rsettrees(r, TREE_SEED, value);
            fset(r, RF_MALLORN);
        }
        else {
            if (r->terrain->production) {
                int i;
                for (i = 0; r->terrain->production[i].type; ++i) {
                    const terrain_production *production = r->terrain->production + i;
                    if (production->type == rtype) {
                        return add_resource(r, 1, value, dice_rand(production->divisor), rtype);
                    }
                }
            }
            /* adamantium etc are not usually terraformed: */
            rm = rm_get(r, rtype);
            if (rm) {
                rm->amount = value;
            }
            else {
                return add_resource(r, 1, value, 150, rtype);
            }
        }
    }
    return rm;
}

int region_getresource_level(const struct region * r, const struct resource_type * rtype)
{
    const rawmaterial *rm = rm_get((struct region *)r, rtype);
    return rm ? rm->level : -1;
}

int region_getresource(const struct region * r, const struct resource_type *rtype)
{
    if (rtype == get_resourcetype(R_SILVER)) {
        return rmoney(r);
    }
    else if (rtype == get_resourcetype(R_HORSE)) {
        return rhorses(r);
    }
    else if (rtype == get_resourcetype(R_PEASANT)) {
        return rpeasants(r);
    }
    else {
        const rawmaterial* rm = rm_get((struct region *)r, rtype);
        if (rm) {
            return rm->amount;
        }
    }
    return 0;
}

void free_region(region * r)
{
    if (last == r)
        last = NULL;
    if (r->land)
        free_land(r->land);

    if (r->msgs) {
        free_messagelist(r->msgs);
        r->msgs = NULL;
    }

    if (r->individual_messages) {
        ptrdiff_t i, l = arrlen(r->individual_messages);
        for (i = 0; i != l; ++i) {
            faction_messages *msg = r->individual_messages + i;
            if (msg->msgs) {
                free_messagelist(msg->msgs);
            }
        }
        arrfree(r->individual_messages);
        r->individual_messages = NULL;
    }

    a_removeall(&r->attribs, NULL);
    arrfree(r->resources);
    r->resources = NULL;

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
    int s, k, e, p = 0, x = 0;
    size_t nk, ne, nv, ns;
    static char name[16];
    const char *kons = "bcdfghklmnprstvwz",
        *handle_start = "bcdgtskpvfr",
        *handle_end = "nlrdst",
        *vowels = "aaaaaaaaaaaeeeeeeeeeeeeiiiiiiiiiiioooooooooooouuuuuuuuuuyy";

    nk = strlen(kons);
    ne = strlen(handle_end);
    nv = strlen(vowels);
    ns = strlen(handle_start);

    for (s = rng_int() % 3 + 2; s > 0; s--) {
        int v;
        if (x > 0) {
            k = rng_int() % (int)nk;
            name[p] = kons[k];
            p++;
        }
        else {
            k = rng_int() % (int)ns;
            name[p] = handle_start[k];
            p++;
        }
        v = rng_int() % (int)nv;
        name[p] = vowels[v];
        p++;
        if (rng_int() % 3 == 2 || s == 1) {
            e = rng_int() % (int)ne;
            name[p] = handle_end[e];
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
    int maxluxuries = get_maxluxuries();
    ptrdiff_t i;

    assert(r->land);

    arrfree(r->land->_demands);
    r->land->_demands = NULL;

    arraddnptr(r->land->_demands, maxluxuries);
    for (i = 0, ltype = luxurytypes; ltype; ++i, ltype = ltype->next) {
        struct demand *dmd = r->land->_demands + i;
        dmd->type = ltype;
        if (ltype != sale) {
            dmd->value = 1 + rng_int() % 5;
        } 
        else {
            dmd->value = 0;
        }
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

void init_region(region *r)
{
    static int changed;
    static const terrain_type *t_plain;
    const terrain_type * terrain = r->terrain;
    int horses = 0, trees = 0;
    if (terrain_changed(&changed)) {
        t_plain = get_terrain(terrainnames[T_PLAIN]);
    }
    if (terrain->size>0) {
        horses = rng_int() % (terrain->size / 50);
        trees = terrain->size * (30 + rng_int() % 40) / 1000;
    }
    if (t_plain && terrain == t_plain) {
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
        int p_wage = 1 + peasant_wage(r, false) + rng_int() % 5;
        peasants = (region_production(r) * (20 + dice(6, 10))) / 100;
        if (peasants < 100) peasants = 100;
        rsetmoney(r, rsetpeasants(r, peasants) * p_wage);
    }
}

static void reset_herbs(region *r) {
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
        rsetherbs(r, 50 + rng_int() % 31);
    }
    else {
        rsetherbtype(r, NULL);
    }

    if (rng_int() % 100 < 3) {
        fset(r, RF_MALLORN);
    }
    else {
        freset(r, RF_MALLORN);
    }
}

void create_land(region *r) {
    if (!r->land) {
        r->land = calloc(1, sizeof(land_region));
    }
}

static void init_land(region *r) {
    struct surround {
        const luxury_type *type;
        int value;
    } *nb = NULL;
    const luxury_type *ltype = NULL;
    int mnr = 0;
    int max_luxuries = get_maxluxuries();
    ptrdiff_t i;

    assert(r);
    assert(r->land);
    assert(r->terrain);
    assert(fval(r->terrain, LAND_REGION));

    r->land->ownership = NULL;
    region_set_morale(r, MORALE_DEFAULT, -1);
    region_setname(r, makename());
    reset_herbs(r);
    fix_demand(r);
    if (r->land->_demands) {
        const struct demand *sale = NULL;
        for (i = arrlen(r->land->_demands) - 1; i >= 0; --i) {
            if (r->land->_demands[i].value == 0) {
                sale = r->land->_demands + i;
            }
        }
        if (sale) {
            direction_t d;
            arrsetcap(nb, max_luxuries);
            for (d = 0; d != MAXDIRECTIONS; ++d) {
                region *nr = rconnect(r, d);
                if (nr && nr->land) {
                    ptrdiff_t i;
                    for (i = arrlen(nb) - 1; i >= 0; --i) {
                        struct surround *sr = nb + i;
                        if (sr->type == sale->type) {
                            ++sr->value;
                            break;
                        }
                    }
                    if (i < 0) {
                        struct surround *sr = arraddnptr(nb, 1);
                        sr->type = sale->type;
                        sr->value = 1;
                    }
                    ++mnr;
                }
            }
        }
    }
    if (!nb) {
        /* TODO: this is really lame */
        int i = max_luxuries;
        if (i > 0) {
            i = rng_int() % i;
            ltype = luxurytypes;
            while (i--)
                ltype = ltype->next;
        }
    }
    else if (mnr > 0) {
        int i = rng_int() % mnr;
        ptrdiff_t p;
        for (p = arrlen(nb) - 1; p >= 0 && i > nb[p].value; --p) {
            i -= nb[p].value;
        }
        if (p >= 0 && nb[p].type) {
            setluxuries(r, nb[p].type);
        }
    }
    arrfree(nb);
}

void terraform_region(region * r, const terrain_type * terrain)
{
    assert(r);
    assert(terrain);

    r->terrain = terrain;
    terraform_resources(r);

    if (!fval(terrain, LAND_REGION)) {
        if (r->land) {
            rsettrees(r, 0, 0);
            rsettrees(r, 1, 0);
            rsettrees(r, 2, 0);
            rsethorses(r, 0);
            rsetpeasants(r, 0);
            rsetmoney(r, 0);
            destroy_all_roads(r);
            free_land(r->land);
            r->land = NULL;
        }
        freset(r, RF_MALLORN);
    }
    else {
        if (!r->land) {
            create_land(r);
            init_land(r);
        }
        else {
            reset_herbs(r);
        }
        init_region(r);
    }
    if (rplane(r) == get_homeplane()) {
        struct plane* aplane = get_astralplane();
        if (aplane) {
            update_teleport_plane(r, aplane, NULL, NULL);
        }
    }

}

/** ENNO:
 * ich denke, das das hier nicht sein sollte.
 * statt dessen sollte ein attribut an der region sein, dass das erledigt,
 * egal ob durch den spell oder anderes angelegt.
 **/
#include "curse.h"
int max_production(const region * r)
{
    /* muss rterrain(r) sein, nicht rterrain() wegen rekursion */
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

int read_region_reference(gamedata * data, region **rp)
{
    struct storage * store = data->store;
    int id = 0;

    READ_INT(store, &id);
    *rp = findregionbyid(id);
    if (*rp == NULL) {
        *rp = region_create(id);
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
    if (r->individual_messages) {
        ptrdiff_t i, l = arrlen(r->individual_messages);
        for (i = 0; i != l; ++i) {
            faction_messages *imsg = r->individual_messages + i;
            if (imsg->viewer == viewer) {
                return imsg->msgs;
            }
        }
    }
    return NULL;
}

struct message *r_addmessage(struct region *r, const struct faction *viewer,
    struct message *msg)
{
    assert(r);
    if (viewer) {
        faction_messages *vmsg = NULL;
        if (r->individual_messages) {
            ptrdiff_t i, l = arrlen(r->individual_messages);
            for (i = 0; i != l; ++i) {
                faction_messages *imsg = r->individual_messages + i;
                if (imsg->viewer == viewer) {
                    vmsg = imsg;
                    break;
                }
            }
        }
        if (!vmsg) {
            vmsg = stbds_arraddnptr(r->individual_messages, 1);
            vmsg->viewer = viewer;
            vmsg->msgs = NULL;
        }
        return add_message(&vmsg->msgs, msg);
    }
    return add_message(&r->msgs, msg);
}

void r_add_warning(region *r, message *msg)
{
    unit *u;
    add_message(&r->msgs, msg);
    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next) {
        if (fval(u->faction, FFL_SELECT))
            continue;
        fset(u->faction, FFL_SELECT);
        add_message(&u->faction->msgs, msg);
    }
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
            region_owner *ro = malloc(sizeof(region_owner));
            if (!ro) abort();
            assert(region_get_morale(r) == MORALE_DEFAULT);
            ro->owner = NULL;
            ro->last_owner = NULL;
            ro->flags = 0;
            r->land->ownership = ro;
        }
        r->land->ownership->since_turn = turn;
        r->land->ownership->morale_turn = turn;
        assert(r->land->ownership->owner != owner);
        r->land->ownership->last_owner = r->land->ownership->owner;
        r->land->ownership->owner = owner;
    }
}

faction *update_region_owners(region * r)
{
    faction *f = NULL;
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
                            int morale = region_get_morale(r) - MORALE_TRANSFER;
                            if (morale < 0) morale = 0;
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
    r->land->display = (info && info[0]) ? str_strdup(info) : 0;
}

const char *region_getinfo(const region * r)
{
    return (r->land && r->land->display) ? r->land->display : "";
}

void region_setname(struct region *r, const char *name)
{
    if (r->land) {
        free(r->land->name);
        r->land->name = name ? str_strdup(name) : 0;
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
        assert(r->land->morale <= MORALE_MAX);
        return r->land->morale;
    }
    return -1;
}

void region_set_morale(region * r, int morale, int turn)
{
    if (r->land) {
        r->land->morale = (unsigned short)morale;
        if (turn >= 0 && r->land->ownership) {
            r->land->ownership->morale_turn = turn;
        }
        assert(r->land->morale <= MORALE_MAX);
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
    int change = owner_change(r) + 1;
    return (change == in_turn && r->land &&
        r->land->ownership->last_owner && r->land->ownership->owner &&
        r->land->ownership->last_owner != r->land->ownership->owner);
}

void destroy_all_roads(region * r)
{
    int i;

    for (i = 0; i < MAXDIRECTIONS; i++) {
        rsetroad(r, (direction_t)i, 0);
    }
}

void reorder_units(region* r)
{
    unit** unext = &r->units;

    if (r->buildings) {
        building* b = r->buildings;
        while (*unext && b) {
            unit** ufirst = unext;    /* where the first unit in the building should go */
            unit** umove = unext;     /* a unit we consider moving */
            unit* owner = building_owner(b);
            while (owner && *umove) {
                unit* u = *umove;
                if (u->building == b) {
                    unit** uinsert = unext;
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
        ship* sh = r->ships;
        /* first, move all units up that are not on ships */
        unit** umove = unext;       /* a unit we consider moving */
        while (*umove) {
            unit* u = *umove;
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
            unit** ufirst = unext;    /* where the first unit in the building should go */
            unit* owner = ship_owner(sh);
            umove = unext;
            while (owner && *umove) {
                unit* u = *umove;
                if (u->number && u->ship == sh) {
                    unit** uinsert = unext;
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

