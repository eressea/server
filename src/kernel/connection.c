#include <kernel/config.h>
#include "connection.h"

#include "move.h"
#include "region.h"
#include "terrain.h"
#include "unit.h"

#include <kernel/attrib.h>
#include <kernel/connection.h>
#include <kernel/gamedata.h>

#include <util/base36.h>
#include <util/language.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/rng.h>

#include <stb_ds.h>
#include <storage.h>
#include <strings.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BORDER_MAXHASH 8191
connection *borders[BORDER_MAXHASH];
border_type *bordertypes;

void(*border_convert_cb) (struct connection * con, struct attrib * attr) = 0;

void free_borders(void)
{
    int i;
    for (i = 0; i != BORDER_MAXHASH; ++i) {
        while (borders[i]) {
            connection *b = borders[i];
            borders[i] = b->nexthash;
            while (b) {
                connection *bf = b;
                b = b->next;
                assert(b == NULL || b->nexthash == NULL);
                if (bf->type->destroy) {
                    bf->type->destroy(bf);
                }
                free(bf);
            }
        }
    }
}

static void walk_i(region *r, connection *b, void(*cb)(connection *, void *), void *data) {
    for (; b; b = b->nexthash) {
        if (b->from == r || b->to == r) {
            connection *bn;
            for (bn = b; bn; bn = bn->next) {
                cb(b, data);
            }
        }
    }
}

void walk_connections(region *r, void(*cb)(connection *, void *), void *data) {
    int key = reg_hashkey(r);
    int d;

    walk_i(r, borders[key], cb, data);
    for (d = 0; d != MAXDIRECTIONS; ++d) {
        region *rn = rconnect(r, d);
        if (rn) {
            int k = reg_hashkey(rn);
            if (k < key) {
                walk_i(r, borders[k], cb, data);
            }
        }
    }
}

static connection **get_borders_i(const region * r1, const region * r2)
{
    connection **bp;
    int key = reg_hashkey(r1);
    int k2 = reg_hashkey(r2);

    if (key > k2) key = k2;
    key = key % BORDER_MAXHASH;
    bp = &borders[key];
    while (*bp) {
        connection *b = *bp;
        if ((b->from == r1 && b->to == r2) || (b->from == r2 && b->to == r1))
            break;
        bp = &b->nexthash;
    }
    return bp;
}

connection *get_borders(const region * r1, const region * r2)
{
    connection **bp = get_borders_i(r1, r2);
    return *bp;
}

connection *border_create(region *from)
{
    connection * b = calloc(1, sizeof(connection));
    if (!b) abort();
    b->from = from;
    return b;
}

connection *create_border(border_type * type, region * from, region * to)
{
    connection *b, **bp;

    assert(from && to);
    bp = get_borders_i(from, to);
    while (*bp) {
        bp = &(*bp)->next;
    }
    *bp = b = border_create(from);
    b->type = type;
    b->to = to;

    if (type->init) {
        type->init(b);
    }
    return b;
}

void erase_border(connection * b)
{
    if (b->from && b->to) {
        connection **bp = get_borders_i(b->from, b->to);
        assert(*bp != NULL || !"error: connection is not registered");
        if (*bp == b) {
            /* it is the first in the list, so it is in the nexthash list */
            if (b->next) {
                *bp = b->next;
                (*bp)->nexthash = b->nexthash;
            }
            else {
                *bp = b->nexthash;
            }
        }
        else {
            while (*bp && *bp != b) {
                bp = &(*bp)->next;
            }
            assert(*bp == b || !"error: connection is not registered");
            *bp = b->next;
        }
    }
    if (b->type->destroy) {
        b->type->destroy(b);
    }
    free(b);
}

void register_bordertype(border_type * type)
{
    border_type **btp = &bordertypes;

    while (*btp && *btp != type)
        btp = &(*btp)->next;
    if (*btp)
        return;
    *btp = type;
}

border_type *find_bordertype(const char *name)
{
    border_type *bt = bordertypes;

    while (bt && strcmp(bt->_name, name)!=0)
        bt = bt->next;
    return bt;
}

void b_read(connection * b, gamedata * data)
{
    storage * store = data->store;
    int n;
    switch (b->type->datatype) {
    case VAR_NONE:
    case VAR_INT:
        READ_INT(store, &b->data.i);
        break;
    case VAR_SHORTA:
        READ_INT(store, &n);
        b->data.sa[0] = (short)n;
        READ_INT(store, &n);
        b->data.sa[1] = (short)n;
        break;
    case VAR_VOIDPTR:
    default:
        assert(!"invalid variant type in connection");
    }
}

void b_write(const connection * b, storage * store)
{
    switch (b->type->datatype) {
    case VAR_NONE:
    case VAR_INT:
        WRITE_INT(store, b->data.i);
        break;
    case VAR_SHORTA:
        WRITE_INT(store, b->data.sa[0]);
        WRITE_INT(store, b->data.sa[1]);
        break;
    case VAR_VOIDPTR:
    default:
        assert(!"invalid variant type in connection");
    }
}

bool b_transparent(const connection * b, const struct faction *f)
{
    UNUSED_ARG(b);
    UNUSED_ARG(f);
    return true;
}

bool b_opaque(const connection * b, const struct faction * f)
{
    UNUSED_ARG(b);
    UNUSED_ARG(f);
    return false;
}

bool b_blockall(const connection * b, const unit * u, const region * r)
{
    UNUSED_ARG(u);
    UNUSED_ARG(r);
    UNUSED_ARG(b);
    return true;
}

bool b_blocknone(const connection * b, const unit * u, const region * r)
{
    UNUSED_ARG(u);
    UNUSED_ARG(r);
    UNUSED_ARG(b);
    return false;
}

bool b_rvisible(const connection * b, const region * r)
{
    return (b->to == r || b->from == r);
}

bool b_fvisible(const connection * b, const struct faction * f,
    const region * r)
{
    UNUSED_ARG(r);
    UNUSED_ARG(f);
    UNUSED_ARG(b);
    return true;
}

bool b_uvisible(const connection * b, const unit * u)
{
    UNUSED_ARG(u);
    UNUSED_ARG(b);
    return true;
}

bool b_rinvisible(const connection * b, const region * r)
{
    UNUSED_ARG(r);
    UNUSED_ARG(b);
    return false;
}

bool b_finvisible(const connection * b, const struct faction * f,
    const region * r)
{
    UNUSED_ARG(r);
    UNUSED_ARG(f);
    UNUSED_ARG(b);
    return false;
}

bool b_uinvisible(const connection * b, const unit * u)
{
    UNUSED_ARG(u);
    UNUSED_ARG(b);
    return false;
}

/**************************************/
/* at_countdown - legacy, do not use  */
/**************************************/

attrib_type at_countdown = {
    "countdown",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    NULL,
    NULL,
    a_readint
};

void age_borders(void)
{
    connection **deleted = NULL;
    int i;

    for (i = 0; i != BORDER_MAXHASH; ++i) {
        connection *bhash = borders[i];
        for (; bhash; bhash = bhash->nexthash) {
            connection *b = bhash;
            for (; b; b = b->next) {
                if (b->type->age) {
                    if (b->type->age(b) == AT_AGE_REMOVE) {
                        arrput(deleted, b);
                    }
                }
            }
        }
    }
    if (deleted) {
        size_t qi, ql;
        for (ql = arrlen(deleted), qi = 0; qi != ql; ++qi) {
            erase_border(deleted[qi]);
        }
        arrfree(deleted);
    }
}

/********
 * implementation of a couple of borders. this shouldn't really be in here, so
 * let's keep it separate from the more general stuff above
 ********/

#include "faction.h"

static const char *b_namewall(const connection * b, const region * r,
    const struct faction *f, int gflags)
{
    const char *bname = "wall";

    UNUSED_ARG(f);
    UNUSED_ARG(r);
    UNUSED_ARG(b);
    if (gflags & GF_ARTICLE)
        bname = "a_wall";
    if (gflags & GF_PURE)
        return bname;
    return LOC(f->locale, mkname("border", bname));
}

border_type bt_wall = {
    "wall", VAR_INT, LAND_REGION,
    b_opaque,
    NULL,                         /* init */
    NULL,                         /* destroy */
    b_read,                       /* read */
    b_write,                      /* write */
    b_blockall,                   /* block */
    b_namewall,                   /* name */
    b_rvisible,                   /* rvisible */
    b_fvisible,                   /* fvisible */
    b_uvisible,                   /* uvisible */
};

border_type bt_noway = {
    "noway", VAR_INT, 0,
    b_transparent,
    NULL,                         /* init */
    NULL,                         /* destroy */
    b_read,                       /* read */
    b_write,                      /* write */
    b_blockall,                   /* block */
    NULL,                         /* name */
    b_rinvisible,                 /* rvisible */
    b_finvisible,                 /* fvisible */
    b_uinvisible,                 /* uvisible */
};

static const char *b_namefogwall(const connection * b, const region * r,
    const struct faction *f, int gflags)
{
    UNUSED_ARG(f);
    UNUSED_ARG(b);
    UNUSED_ARG(r);
    if (gflags & GF_PURE)
        return "fogwall";
    if (gflags & GF_ARTICLE)
        return LOC(f->locale, mkname("border", "a_fogwall"));
    return LOC(f->locale, mkname("border", "fogwall"));
}

static bool
b_blockfogwall(const connection * b, const unit * u, const region * r)
{
    UNUSED_ARG(b);
    if (!u)
        return true;
    return (effskill(u, SK_PERCEPTION, r) > 4);    /* Das ist die alte Nebelwand */
}

/** Legacy type used in old Eressea games, no longer in use. */
border_type bt_fogwall = {
    "fogwall", VAR_INT, 0,
    b_transparent,                /* transparent */
    NULL,                         /* init */
    NULL,                         /* destroy */
    b_read,                       /* read */
    b_write,                      /* write */
    b_blockfogwall,               /* block */
    b_namefogwall,                /* name */
    b_rvisible,                   /* rvisible */
    b_fvisible,                   /* fvisible */
    b_uvisible,                   /* uvisible */
};

static void b_readroad(connection * b, gamedata * data)
{
    storage * store = data->store;
    int n;
    direction_t dir;
    assert(b->from && b->to);
    READ_INT(store, &n);
	dir = reldirection(b->from, b->to);
	rsetroad(b->from, dir, n);
    READ_INT(store, &n);
	dir = reldirection(b->to, b->from);
	rsetroad(b->to, dir, n);
}

border_type bt_road = {
    "road", VAR_NONE, LAND_REGION,
    NULL,
    NULL,                         /* init */
    NULL,                         /* destroy */
    b_readroad,                   /* read */
};

void write_borders(struct storage *store)
{
    int i;
    for (i = 0; i != BORDER_MAXHASH; ++i) {
        connection *bhash;
        for (bhash = borders[i]; bhash; bhash = bhash->nexthash) {
            connection *b;
            for (b = bhash; b != NULL; b = b->next) {
                if (b->type->valid && !b->type->valid(b))
                    continue;
                WRITE_TOK(store, b->type->_name);
                WRITE_INT(store, b->from->uid);
                WRITE_INT(store, b->to->uid);

                if (b->type->write)
                    b->type->write(b, store);
                WRITE_SECTION(store);
            }
        }
    }
    WRITE_TOK(store, "end");
}

int read_borders(gamedata *data)
{
    struct storage *store = data->store;
    for (;;) {
        int fid, tid;
        char zText[32];
        region *from, *to;
        border_type *type;
        connection dummy;

        READ_TOK(store, zText, sizeof(zText));
        if (!strcmp(zText, "end")) {
            break;
        }
        if (data->version < BORDER_ID_VERSION) {
            READ_INT(store, NULL);
        }
        type = find_bordertype(zText);
        if (type == NULL) {
            log_error("[read_borders] connection type %s is not registered", zText);
            assert(type || !"connection type not registered");
        }

        READ_INT(store, &fid);
        READ_INT(store, &tid);
        from = dummy.from = findregionbyid(fid);
        to = dummy.to = findregionbyid(tid);
        if (!to || !from) {
            log_error("%s connection from %d to %d has missing regions", zText, fid, tid);
            if (type->read) {
                /* skip ahead */
                type->read(&dummy, data);
            }
            continue;
        }

        if (to == from && from) {
            direction_t dir = (direction_t)(rng_int() % MAXDIRECTIONS);
            region *r = rconnect(from, dir);
            log_error("[read_borders] invalid %s in %s\n", type->_name, regionname(from, NULL));
            if (r != NULL)
                to = r;
        }
        if (type->read) {
            connection *b = NULL;
            
            if (data->version < FIX_SEAROADS_VERSION) {
                /* bug 2694: eliminate roads in oceans */
                if (type->terrain_flags != 0 && type->terrain_flags != fval(from->terrain, type->terrain_flags)) {
                    log_info("ignoring %s connection in %s", type->_name, from->terrain->_name);
                    b = &dummy;
                }
            }
            if (b == NULL) {
				if (type == &bt_road) {
					/* delete old bt_road instances */
					b = &dummy;
				}
				else {
					b = create_border(type, from, to);
				}
            }
            type->read(b, data);
            if (type->datatype != VAR_NONE && !type->write) {
                log_warning("invalid border '%s' between '%s' and '%s'\n", zText, regionname(from, 0), regionname(to, 0));
            }
        }
    }
    return 0;
}

const char * border_name(const connection *co, const struct region * r, const struct faction * f, int flags, const struct locale *lang) {
    const char * bname = (co->type->name) ? co->type->name(co, r, f, flags) : co->type->_name;
    if (lang) {
        const char *result = mkname("border", bname);
        result = LOC(f->locale, result);
        if (result) {
            return result;
        }
    }
    return bname;
}

void register_connections(void)
{
    /* connection-typen */
    register_bordertype(&bt_noway);
    register_bordertype(&bt_fogwall);
    register_bordertype(&bt_wall);
    register_bordertype(&bt_road);
}
