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

#ifdef _MSC_VER
#include <platform.h>
#endif
#include <kernel/config.h>
#include "connection.h"

#include "region.h"
#include "terrain.h"
#include "unit.h"

#include <util/attrib.h>
#include <util/base36.h>
#include <util/gamedata.h>
#include <util/language.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/rng.h>
#include <util/strings.h>

#include <selist.h>
#include <storage.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int nextborder = 0;

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

connection *find_border(int id)
{
    int key;
    for (key = 0; key != BORDER_MAXHASH; key++) {
        connection *bhash;
        for (bhash = borders[key]; bhash != NULL; bhash = bhash->nexthash) {
            connection *b;
            for (b = bhash; b; b = b->next) {
                if (b->id == id)
                    return b;
            }
        }
    }
    return NULL;
}

int resolve_borderid(variant id, void *addr)
{
    int result = 0;
    connection *b = NULL;
    if (id.i != 0) {
        b = find_border(id.i);
        if (b == NULL) {
            result = -1;
        }
    }
    *(connection **)addr = b;
    return result;
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
        region *rn = r_connect(r, d);
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

connection *new_border(border_type * type, region * from, region * to)
{
    connection *b, **bp = get_borders_i(from, to);

    assert(from && to);
    while (*bp) {
        bp = &(*bp)->next;
    }
    *bp = b = calloc(1, sizeof(connection));
    b->type = type;
    b->from = from;
    b->to = to;
    b->id = ++nextborder;

    if (type->init)
        type->init(b);
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

    while (bt && strcmp(bt->__name, name)!=0)
        bt = bt->next;
    return bt;
}

void b_read(connection * b, gamedata * data)
{
    storage * store = data->store;
    int n, result = 0;
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
        result = 0;
    }
    assert(result >= 0 || "EOF encountered?");
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
    return (bool)(b->to == r || b->from == r);
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
    selist *deleted = NULL, *ql;
    int i;

    for (i = 0; i != BORDER_MAXHASH; ++i) {
        connection *bhash = borders[i];
        for (; bhash; bhash = bhash->nexthash) {
            connection *b = bhash;
            for (; b; b = b->next) {
                if (b->type->age) {
                    if (b->type->age(b) == AT_AGE_REMOVE) {
                        selist_push(&deleted, b);
                    }
                }
            }
        }
    }
    for (ql = deleted, i = 0; ql; selist_advance(&ql, &i, 1)) {
        connection *b = (connection *)selist_get(ql, i);
        erase_border(b);
    }
    selist_free(deleted);
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
    "wall", VAR_INT,
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
    "noway", VAR_INT,
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
    return (bool)(effskill(u, SK_PERCEPTION, r) > 4);    /* Das ist die alte Nebelwand */
}

/** Legacy type used in old Eressea games, no longer in use. */
border_type bt_fogwall = {
    "fogwall", VAR_INT,
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

static const char *b_nameillusionwall(const connection * b, const region * r,
    const struct faction *f, int gflags)
{
    int fno = b->data.i;
    UNUSED_ARG(b);
    UNUSED_ARG(r);
    if (gflags & GF_PURE)
        return (f && fno == f->no) ? "illusionwall" : "wall";
    if (gflags & GF_ARTICLE) {
        return LOC(f->locale, mkname("border", (f
            && fno == f->subscription) ? "an_illusionwall" : "a_wall"));
    }
    return LOC(f->locale, mkname("border", (f
        && fno == f->no) ? "illusionwall" : "wall"));
}

border_type bt_illusionwall = {
    "illusionwall", VAR_INT,
    b_opaque,
    NULL,                         /* init */
    NULL,                         /* destroy */
    b_read,                       /* read */
    b_write,                      /* write */
    b_blocknone,                  /* block */
    b_nameillusionwall,           /* name */
    b_rvisible,                   /* rvisible */
    b_fvisible,                   /* fvisible */
    b_uvisible,                   /* uvisible */
};

/***
 * roads. meant to replace the old at_road or r->road attribute
 ***/

static const char *b_nameroad(const connection * b, const region * r,
    const struct faction *f, int gflags)
{
    region *r2 = (r == b->to) ? b->from : b->to;
    int local = (r == b->from) ? b->data.sa[0] : b->data.sa[1];
    static char buffer[64];

    UNUSED_ARG(f);
    if (gflags & GF_PURE)
        return "road";
    if (gflags & GF_ARTICLE) {
        if (!(gflags & GF_DETAILED))
            return LOC(f->locale, mkname("border", "a_road"));
        else if (r->terrain->max_road <= local) {
            int remote = (r2 == b->from) ? b->data.sa[0] : b->data.sa[1];
            if (r2->terrain->max_road <= remote) {
                return LOC(f->locale, mkname("border", "a_road"));
            }
            else {
                return LOC(f->locale, mkname("border", "an_incomplete_road"));
            }
        }
        else {
            if (local) {
                const char *temp = LOC(f->locale, mkname("border", "a_road_percent"));
                int percent = 100 * local / r->terrain->max_road;
                if (percent < 1) percent = 1;
                str_replace(buffer, sizeof(buffer), temp, "$percent", itoa10(percent));
            }
            else {
                return LOC(f->locale, mkname("border", "a_road_connection"));
            }
        }
    }
    else if (gflags & GF_PLURAL)
        return LOC(f->locale, mkname("border", "roads"));
    else
        return LOC(f->locale, mkname("border", "road"));
    return buffer;
}

static void b_readroad(connection * b, gamedata * data)
{
    storage * store = data->store;
    int n;
    READ_INT(store, &n);
    b->data.sa[0] = (short)n;
    READ_INT(store, &n);
    b->data.sa[1] = (short)n;
}

static void b_writeroad(const connection * b, storage * store)
{
    WRITE_INT(store, b->data.sa[0]);
    WRITE_INT(store, b->data.sa[1]);
}

static bool b_validroad(const connection * b)
{
    return (b->data.sa[0] != SHRT_MAX);
}

static bool b_rvisibleroad(const connection * b, const region * r)
{
    int x = b->data.i;
    x = (r == b->from) ? b->data.sa[0] : b->data.sa[1];
    if (x == 0) {
        return false;
    }
    if (b->to != r && b->from != r) {
        return false;
    }
    return true;
}

border_type bt_road = {
    "road", VAR_INT,
    b_transparent,
    NULL,                         /* init */
    NULL,                         /* destroy */
    b_readroad,                   /* read */
    b_writeroad,                  /* write */
    b_blocknone,                  /* block */
    b_nameroad,                   /* name */
    b_rvisibleroad,               /* rvisible */
    b_finvisible,                 /* fvisible */
    b_uinvisible,                 /* uvisible */
    b_validroad                   /* valid */
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
                WRITE_TOK(store, b->type->__name);
                WRITE_INT(store, b->id);
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
        int fid, tid, bid;
        char zText[32];
        region *from, *to;
        border_type *type;

        READ_TOK(store, zText, sizeof(zText));
        if (!strcmp(zText, "end")) {
            break;
        }
        READ_INT(store, &bid);
        type = find_bordertype(zText);
        if (type == NULL) {
            log_error("[read_borders] connection %d type %s is not registered", bid, zText);
            assert(type || !"connection type not registered");
        }

        READ_INT(store, &fid);
        READ_INT(store, &tid);
        from = findregionbyid(fid);
        to = findregionbyid(tid);
        if (!to || !from) {
            log_error("%s connection %d has missing regions", zText, bid);
            if (type->read) {
                /* skip ahead */
                connection dummy;
                type->read(&dummy, data);
            }
            continue;
        }

        if (to == from && from) {
            direction_t dir = (direction_t)(rng_int() % MAXDIRECTIONS);
            region *r = rconnect(from, dir);
            log_error("[read_borders] invalid %s in %s\n", type->__name, regionname(from, NULL));
            if (r != NULL)
                to = r;
        }
        if (type->read) {
            connection *b = new_border(type, from, to);
            nextborder--;               /* new_border erh�ht den Wert */
            b->id = bid;
            assert(bid <= nextborder);
            type->read(b, data);
            if (!type->write) {
                log_warning("invalid border '%s' between '%s' and '%s'\n", zText, regionname(from, 0), regionname(to, 0));
            }
        }
    }
    return 0;
}

const char * border_name(const connection *co, const struct region * r, const struct faction * f, int flags) {
    return (co->type->name) ? co->type->name(co, r, f, flags) : co->type->__name;
}
