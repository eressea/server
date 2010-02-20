/* vi: set ts=2:
 *
 *  
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <platform.h>
#include <kernel/eressea.h>
#include "connection.h"

#include "region.h"
#include "save.h"
#include "terrain.h"
#include "unit.h"
#include "version.h"

#include <util/attrib.h>
#include <util/language.h>
#include <util/log.h>
#include <util/rng.h>
#include <util/storage.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <string.h>

unsigned int nextborder = 0;

#define BORDER_MAXHASH 8191
connection * borders[BORDER_MAXHASH];
border_type * bordertypes;

void (*border_convert_cb)(struct connection * con, struct attrib * attr) = 0;

void
free_borders(void)
{
  int i;
  for (i=0;i!=BORDER_MAXHASH;++i) {
    while (borders[i]) {
      connection * b = borders[i];
      borders[i] = b->nexthash;
      while (b) {
        connection * bf = b;
        b = b->next;
        assert(b==NULL || b->nexthash==NULL);
        if (bf->type->destroy) {
          bf->type->destroy(bf);
        }
        free(bf);
      }
    }
  }
}

connection *
find_border(unsigned int id)
{
  int key;
  for (key=0;key!=BORDER_MAXHASH;key++) {
    connection * bhash;
    for (bhash=borders[key];bhash!=NULL;bhash=bhash->nexthash) {
      connection * b;
      for (b=bhash;b;b=b->next) {
        if (b->id==id) return b;
      }
    }
  }
  return NULL;
}

int
resolve_borderid(variant id, void * addr)
{
  int result = 0;
  connection * b = NULL;
  if (id.i!=0) {
    b = find_border(id.i);
    if (b==NULL) {
      result = -1;
    }
  }
  *(connection**)addr = b;
  return result;
}

static connection **
get_borders_i(const region * r1, const region * r2)
{
  connection ** bp;
  int key = reg_hashkey(r1);
  int k2 = reg_hashkey(r2);

  key = MIN(k2, key) % BORDER_MAXHASH;
  bp = &borders[key];
  while (*bp) {
    connection * b = *bp;
    if ((b->from==r1 && b->to==r2) || (b->from==r2 && b->to==r1)) break;
    bp = &b->nexthash;
  }
  return bp;
}

connection *
get_borders(const region * r1, const region * r2)
{
  connection ** bp = get_borders_i(r1, r2);
  return *bp;
}

connection *
new_border(border_type * type, region * from, region * to)
{
  connection * b = calloc(1, sizeof(struct connection));

  if (from && to) {
    connection ** bp = get_borders_i(from, to);
    while (*bp) bp = &(*bp)->next;
    *bp = b;
  }
  b->type = type;
  b->from = from;
  b->to = to;
  b->id = ++nextborder;

  if (type->init) type->init(b);
  return b;
}

void
erase_border(connection * b)
{
  if (b->from && b->to) {
    connection ** bp = get_borders_i(b->from, b->to);
    assert(*bp!=NULL || !"error: connection is not registered");
    if (*bp==b) {
      /* it is the first in the list, so it is in the nexthash list */
      if (b->next) {
        *bp = b->next;
        (*bp)->nexthash = b->nexthash;
      } else {
        *bp = b->nexthash;
      }
    } else {
      while (*bp && *bp != b) {
        bp = &(*bp)->next;
      }
      assert(*bp==b || !"error: connection is not registered");
      *bp = b->next;
    }
  }
  if (b->type->destroy) {
    b->type->destroy(b);
  }
  free(b);
}

void
register_bordertype(border_type * type)
{
  border_type ** btp = &bordertypes;

  while (*btp && *btp!=type) btp = &(*btp)->next;
  if (*btp) return;
  *btp = type;
}


border_type *
find_bordertype(const char * name)
{
  border_type * bt = bordertypes;

  while (bt && strcmp(bt->__name, name)) bt = bt->next;
  return bt;
}

void
b_read(connection * b, storage * store)
{
  int result = 0;
  switch (b->type->datatype) {
    case VAR_NONE:
    case VAR_INT:
      b->data.i = store->r_int(store);
      break;
    case VAR_SHORTA:
      b->data.sa[0] = (short)store->r_int(store);
      b->data.sa[1] = (short)store->r_int(store);
      break;
    case VAR_VOIDPTR:
    default:
      assert(!"invalid variant type in connection");
      result = 0;
  }
  assert(result>=0 || "EOF encountered?");
}

void
b_write(const connection * b, storage * store)
{
  switch (b->type->datatype) {
    case VAR_NONE:
    case VAR_INT:
      store->w_int(store, b->data.i);
      break;
    case VAR_SHORTA:
      store->w_int(store, b->data.sa[0]);
      store->w_int(store, b->data.sa[1]);
      break;
    case VAR_VOIDPTR:
    default:
      assert(!"invalid variant type in connection");
  }
}

boolean b_transparent(const connection * b, const struct faction * f) { unused(b); unused(f); return true; }
boolean b_opaque(const connection * b, const struct faction * f) { unused(b); unused(f); return false; }
boolean b_blockall(const connection * b, const unit * u, const region * r) { unused(u); unused(r); unused(b); return true; }
boolean b_blocknone(const connection * b, const unit * u, const region * r) { unused(u); unused(r); unused(b); return false; }
boolean b_rvisible(const connection * b, const region * r) { return (boolean)(b->to==r || b->from==r); }
boolean b_fvisible(const connection * b, const struct faction * f, const region * r) { unused(r); unused(f); unused(b); return true; }
boolean b_uvisible(const connection * b, const unit * u)  { unused(u); unused(b); return true; }
boolean b_rinvisible(const connection * b, const region * r) { unused(r); unused(b); return false; }
boolean b_finvisible(const connection * b, const struct faction * f, const region * r) { unused(r); unused(f); unused(b); return false; }
boolean b_uinvisible(const connection * b, const unit * u) { unused(u); unused(b); return false; }

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

void
age_borders(void)
{
  border_list * deleted = NULL;
  int i;

  for (i=0;i!=BORDER_MAXHASH;++i) {
    connection * bhash = borders[i];
    for (;bhash;bhash=bhash->nexthash) {
      connection * b = bhash;
      for (;b;b=b->next) {
        if (b->type->age) {
          if (b->type->age(b)==AT_AGE_REMOVE) {
            border_list * kill = malloc(sizeof(border_list));
            kill->data = b;
            kill->next = deleted;
            deleted = kill;
          }
        }
      }
    }
  }
  while (deleted) {
    border_list * blist = deleted->next;
    connection * b = deleted->data;
    erase_border(b);
    free(deleted);
    deleted = blist;
  }
}

/********
 * implementation of a couple of borders. this shouldn't really be in here, so
 * let's keep it separate from the more general stuff above
 ********/

#include "faction.h"

static const char *
b_namewall(const connection * b, const region * r, const struct faction * f, int gflags)
{
  const char * bname = "wall";

  unused(f);
  unused(r);
  unused(b);
  if (gflags & GF_ARTICLE) bname = "a_wall";
  if (gflags & GF_PURE) return bname;
  return LOC(f->locale, mkname("border", bname));
}

border_type bt_wall = {
  "wall", VAR_INT,
  b_opaque,
  NULL, /* init */
  NULL, /* destroy */
  b_read, /* read */
  b_write, /* write */
  b_blockall, /* block */
  b_namewall, /* name */
  b_rvisible, /* rvisible */
  b_fvisible, /* fvisible */
  b_uvisible, /* uvisible */
};

border_type bt_noway = {
  "noway", VAR_INT,
  b_transparent,
  NULL, /* init */
  NULL, /* destroy */
  b_read, /* read */
  b_write, /* write */
  b_blockall, /* block */
  NULL, /* name */
  b_rinvisible, /* rvisible */
  b_finvisible, /* fvisible */
  b_uinvisible, /* uvisible */
};

static const char *
b_namefogwall(const connection * b, const region * r, const struct faction * f, int gflags)
{
  unused(f);
  unused(b);
  unused(r);
  if (gflags & GF_PURE) return "fogwall";
  if (gflags & GF_ARTICLE) return LOC(f->locale, mkname("border", "a_fogwall"));
  return LOC(f->locale, mkname("border", "fogwall"));
}

static boolean
b_blockfogwall(const connection * b, const unit * u, const region * r)
{
  unused(b);
  unused(r);
  if (!u) return true;
  return (boolean)(effskill(u, SK_PERCEPTION) > 4); /* Das ist die alte Nebelwand */
}

/** Legacy type used in old Eressea games, no longer in use. */
border_type bt_fogwall = {
  "fogwall", VAR_INT,
  b_transparent, /* transparent */
  NULL, /* init */
  NULL, /* destroy */
  b_read, /* read */
  b_write, /* write */
  b_blockfogwall, /* block */
  b_namefogwall, /* name */
  b_rvisible, /* rvisible */
  b_fvisible, /* fvisible */
  b_uvisible, /* uvisible */
};

static const char *
b_nameillusionwall(const connection * b, const region * r, const struct faction * f, int gflags)
{
  int fno = b->data.i;
  unused(b);
  unused(r);
  if (gflags & GF_PURE) return (f && fno==f->no)?"illusionwall":"wall";
  if (gflags & GF_ARTICLE) {
    return LOC(f->locale, mkname("border", (f && fno==f->subscription)?"an_illusionwall":"a_wall"));
  }
  return LOC(f->locale, mkname("border", (f && fno==f->no)?"illusionwall":"wall"));
}

border_type bt_illusionwall = {
  "illusionwall", VAR_INT,
  b_opaque,
  NULL, /* init */
  NULL, /* destroy */
  b_read, /* read */
  b_write, /* write */
  b_blocknone, /* block */
  b_nameillusionwall, /* name */
  b_rvisible, /* rvisible */
  b_fvisible, /* fvisible */
  b_uvisible, /* uvisible */
};

/***
 * special quest door
 ***/

boolean b_blockquestportal(const connection * b, const unit * u, const region * r) {
  if(b->data.i > 0) return true;
  return false;
}

static const char *
b_namequestportal(const connection * b, const region * r, const struct faction * f, int gflags)
{
  const char * bname;
  int lock = b->data.i;
  unused(b);
  unused(r);

  if (gflags & GF_ARTICLE) {
    if (lock > 0) {
      bname = "a_gate_locked";
    } else {
      bname = "a_gate_open";
    }
  } else {
    if (lock > 0) {
      bname = "gate_locked";
    } else {
      bname = "gate_open";
    }
  }
  if (gflags & GF_PURE) return bname;
  return LOC(f->locale, mkname("border", bname));
}

border_type bt_questportal = {
  "questportal", VAR_INT,
  b_opaque,
  NULL, /* init */
  NULL, /* destroy */
  b_read, /* read */
  b_write, /* write */
  b_blockquestportal, /* block */
  b_namequestportal, /* name */
  b_rvisible, /* rvisible */
  b_fvisible, /* fvisible */
  b_uvisible, /* uvisible */
};

/***
 * roads. meant to replace the old at_road or r->road attribute
 ***/

static const char *
b_nameroad(const connection * b, const region * r, const struct faction * f, int gflags)
{
  region * r2 = (r==b->to)?b->from:b->to;
  int local = (r==b->from)?b->data.sa[0]:b->data.sa[1];
  static char buffer[64];

  unused(f);
  if (gflags & GF_PURE) return "road";
  if (gflags & GF_ARTICLE) {
    if (!(gflags & GF_DETAILED)) return LOC(f->locale, mkname("border", "a_road"));
    else if (r->terrain->max_road<=local) {
      int remote = (r2==b->from)?b->data.sa[0]:b->data.sa[1];
      if (r2->terrain->max_road<=remote) {
        return LOC(f->locale, mkname("border", "a_road"));
      } else {
        return LOC(f->locale, mkname("border", "an_incomplete_road"));
      }
    } else {
      int percent = MAX(1, 100*local/r->terrain->max_road);
      if (local) {
        snprintf(buffer, sizeof(buffer), LOC(f->locale, mkname("border", "a_road_percent")), percent);
      } else {
        return LOC(f->locale, mkname("border", "a_road_connection"));
      }
    }
  }
  else if (gflags & GF_PLURAL) return LOC(f->locale, mkname("border", "roads"));
  else return LOC(f->locale, mkname("border", "road"));
  return buffer;
}

static void
b_readroad(connection * b, storage * store)
{
  b->data.sa[0] = (short)store->r_int(store);
  b->data.sa[1] = (short)store->r_int(store);
}

static void
b_writeroad(const connection * b, storage * store)
{
  store->w_int(store, b->data.sa[0]);
  store->w_int(store, b->data.sa[1]);
}

static boolean
b_validroad(const connection * b)
{
  if (b->data.sa[0]==SHRT_MAX) return false;
  return true;
}

static boolean
b_rvisibleroad(const connection * b, const region * r)
{
  int x = b->data.i;
  x = (r==b->from)?b->data.sa[0]:b->data.sa[1];
  if (x==0) {
    return false;
  }
  if (b->to!=r && b->from!=r) {
    return false;
  }
  return true;
}

border_type bt_road = {
  "road", VAR_INT,
  b_transparent,
  NULL, /* init */
  NULL, /* destroy */
  b_readroad, /* read */
  b_writeroad, /* write */
  b_blocknone, /* block */
  b_nameroad, /* name */
  b_rvisibleroad, /* rvisible */
  b_finvisible, /* fvisible */
  b_uinvisible, /* uvisible */
  b_validroad /* valid */
};

void
write_borders(struct storage * store)
{
  int i;
  for (i=0;i!=BORDER_MAXHASH;++i) {
    connection * bhash;
    for (bhash=borders[i];bhash;bhash=bhash->nexthash) {
      connection * b;
      for (b=bhash;b!=NULL;b=b->next) {
        if (b->type->valid && !b->type->valid(b)) continue;
        store->w_tok(store, b->type->__name);
        store->w_int(store, b->id);
        store->w_int(store, b->from->uid);
        store->w_int(store, b->to->uid);

        if (b->type->write) b->type->write(b, store);
        store->w_brk(store);
      }
    }
  }
  store->w_tok(store, "end");
}

int
read_borders(struct storage * store)
{
  for (;;) {
    unsigned int bid = 0;
    char zText[32];
    connection * b;
    region * from, * to;
    border_type * type;

    store->r_tok_buf(store, zText, sizeof(zText));
    if (!strcmp(zText, "end")) break;
    bid = store->r_int(store);
    if (store->version<UIDHASH_VERSION) {
      short fx, fy, tx, ty;
      fx = (short)store->r_int(store);
      fy = (short)store->r_int(store);
      tx = (short)store->r_int(store);
      ty = (short)store->r_int(store);
      from = findregion(fx, fy);
      to = findregion(tx, ty);
    } else {
      unsigned int fid = (unsigned int)store->r_int(store);
      unsigned int tid = (unsigned int)store->r_int(store);
      from = findregionbyid(fid);
      to = findregionbyid(tid);
    }

    type = find_bordertype(zText);
    if (type==NULL) {
      log_error(("[read_borders] unknown connection type %s in %s\n", zText, 
        regionname(from, NULL)));
      assert(type || !"connection type not registered");
    }

    if (to==from && type && from) {
      direction_t dir = (direction_t) (rng_int() % MAXDIRECTIONS);
      region * r = rconnect(from, dir);
      log_error(("[read_borders] invalid %s in %s\n", type->__name, 
        regionname(from, NULL)));
      if (r!=NULL) to = r;
    }
    b = new_border(type, from, to);
    nextborder--; /* new_border erhöht den Wert */
    b->id = bid;
    assert(bid<=nextborder);
    if (type->read) type->read(b, store);
    if (store->version<NOBORDERATTRIBS_VERSION) {
      attrib * a = NULL;
      int result = a_read(store, &a);
      if (border_convert_cb) border_convert_cb(b, a);
      while (a) {
        a_remove(&a, a);
      }
      if (result<0) return result;
    }
    if (!to || !from) {
      erase_border(b);
    }
  }
  return 0;
}
