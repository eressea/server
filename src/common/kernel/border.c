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

#include <config.h>
#include "eressea.h"
#include "border.h"

#include "region.h"
#include "save.h"
#include "terrain.h"
#include "unit.h"

#include <util/attrib.h>
#include <util/log.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <string.h>

extern boolean incomplete_data;

unsigned int nextborder = 0;

border * borders[BMAXHASH];
border_type * bordertypes;


void
free_borders(void)
{
  int i;
  for (i=0;i!=BMAXHASH;++i) {
    borders[i] = NULL;
  }
}

border *
find_border(unsigned int id)
{
  int key;
  for (key=0;key!=BMAXHASH;key++) {
    border * bhash;
    for (bhash=borders[key];bhash!=NULL;bhash=bhash->nexthash) {
      border * b;
      for (b=bhash;b;b=b->next) {
        if (b->id==id) return b;
      }
    }
  }
  return NULL;
}

void *
resolve_borderid(variant id) {
   return (void*)find_border(id.i);
}

static border **
get_borders_i(const region * r1, const region * r2)
{
  border ** bp;
  int key = reg_hashkey(r1);
  int k2 = reg_hashkey(r2);

  key = min(k2, key) % BMAXHASH;
  bp = &borders[key];
  while (*bp) {
    border * b = *bp;
    if ((b->from==r1 && b->to==r2) || (b->from==r2 && b->to==r1)) break;
    bp = &b->nexthash;
  }
  return bp;
}

border *
get_borders(const region * r1, const region * r2)
{
  border ** bp = get_borders_i(r1, r2);
  return *bp;
}

border *
new_border(border_type * type, region * from, region * to)
{
  border * b = calloc(1, sizeof(struct border));

  if (from && to) {
    border ** bp = get_borders_i(from, to);
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
erase_border(border * b)
{
  attrib ** ap = &b->attribs;

  while (*ap) a_remove(&b->attribs, *ap);

  if (b->from && b->to) {
    border ** bp = get_borders_i(b->from, b->to);
    assert(*bp!=NULL || !"error: border is not registered");
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
      assert(*bp==b || !"error: border is not registered");
      *bp = b->next;
    }
  }
  if (b->type->destroy) b->type->destroy(b);
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
b_read(border * b, FILE *f)
{
  int result;
  switch (b->type->datatype) {
    case VAR_NONE:
    case VAR_INT:
      result = fscanf(f, "%x ", &b->data.i);
      break;
    case VAR_VOIDPTR:
      result = fscanf(f, "%p ", &b->data.v);
      break;
    case VAR_SHORTA:
      result = fscanf(f, "%hd %hd ", &b->data.sa[0], &b->data.sa[1]);
      break;
    default:
      assert(!"unhandled variant type in border");
      result = 0;
  }
  assert(result>=0 || "EOF encountered?");
}

void
b_write(const border * b, FILE *f)
{
  switch (b->type->datatype) {
    case VAR_NONE:
    case VAR_INT:
      fprintf(f, "%x ", b->data.i);
      break;
    case VAR_VOIDPTR:
      fprintf(f, "%p ", b->data.v);
      break;
    case VAR_SHORTA:
      fprintf(f, "%d %d ", b->data.sa[0], b->data.sa[1]);
      break;
    default:
      assert(!"unhandled variant type in border");
  }
}

boolean b_transparent(const border * b, const struct faction * f) { unused(b); unused(f); return true; }
boolean b_opaque(const border * b, const struct faction * f) { unused(b); unused(f); return false; }
boolean b_blockall(const border * b, const unit * u, const region * r) { unused(u); unused(r); unused(b); return true; }
boolean b_blocknone(const border * b, const unit * u, const region * r) { unused(u); unused(r); unused(b); return false; }
boolean b_rvisible(const border * b, const region * r) { return (boolean)(b->to==r || b->from==r); }
boolean b_fvisible(const border * b, const struct faction * f, const region * r) { unused(r); unused(f); unused(b); return true; }
boolean b_uvisible(const border * b, const unit * u)  { unused(u); unused(b); return true; }
boolean b_rinvisible(const border * b, const region * r) { unused(r); unused(b); return false; }
boolean b_finvisible(const border * b, const struct faction * f, const region * r) { unused(r); unused(f); unused(b); return false; }
boolean b_uinvisible(const border * b, const unit * u) { unused(u); unused(b); return false; }

/*********************/
/*   at_countdown   */
/*********************/

static int
a_agecountdown(attrib * a)
{
  a->data.i = max(a->data.i-1, 0);
  return a->data.i;
}

attrib_type at_countdown = {
  "countdown",
  DEFAULT_INIT,
  DEFAULT_FINALIZE,
  a_agecountdown,
  a_writeint,
  a_readint
};

void
age_borders(void)
{
  border_list * deleted = NULL;
  int i;

  for (i=0;i!=BMAXHASH;++i) {
    border * bhash = borders[i];
    for (;bhash;bhash=bhash->nexthash) {
      border * b = bhash;
      for (;b;b=b->next) {
        attrib ** ap = &b->attribs;
        while (*ap) {
          attrib * a = *ap;
          if (a->type->age && a->type->age(a)==0) {
            if (a->type == &at_countdown) {
              border_list * bnew = malloc(sizeof(border_list));
              bnew->next = deleted;
              bnew->data = b;
              deleted = bnew;
              break;
            }
            a_remove(&b->attribs, a);
          }
          else ap=&a->next;
        }
      }
    }
  }
  while (deleted) {
    border_list * blist = deleted->next;
    border * b = deleted->data;
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
b_namewall(const border * b, const region * r, const struct faction * f, int gflags)
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
b_namefogwall(const border * b, const region * r, const struct faction * f, int gflags)
{
  unused(f);
  unused(b);
  unused(r);
  if (gflags & GF_PURE) return "fogwall";
  if (gflags & GF_ARTICLE) return LOC(f->locale, mkname("border", "a_fogwall"));
  return LOC(f->locale, mkname("border", "fogwall"));
}

static boolean
b_blockfogwall(const border * b, const unit * u, const region * r)
{
  unused(b);
  unused(r);
  if (!u) return true;
  return (boolean)(effskill(u, SK_OBSERVATION) > 4); /* Das ist die alte Nebelwand */
}

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
b_nameillusionwall(const border * b, const region * r, const struct faction * f, int gflags)
{
  /* TODO: UNICODE: f->locale bestimmt die Sprache */
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

boolean b_blockquestportal(const border * b, const unit * u, const region * r) {
  if(b->data.i > 0) return true;
  return false;
}

static const char *
b_namequestportal(const border * b, const region * r, const struct faction * f, int gflags)
{
  /* TODO: UNICODE: f->locale bestimmt die Sprache */
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
b_nameroad(const border * b, const region * r, const struct faction * f, int gflags)
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
      int percent = max(1, 100*local/r->terrain->max_road);
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
b_readroad(border * b, FILE *f)
{
  fscanf(f, "%hd %hd ", &b->data.sa[0], &b->data.sa[1]);
}

static void
b_writeroad(const border * b, FILE *f)
{
  fprintf(f, "%d %d ", b->data.sa[0], b->data.sa[1]);
}

static boolean
b_validroad(const border * b)
{
  if (b->data.sa[0]==SHRT_MAX) return false;
  return true;
}

static boolean
b_rvisibleroad(const border * b, const region * r)
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
write_borders(FILE * f)
{
  int i;
  for (i=0;i!=BMAXHASH;++i) {
    border * bhash;
    for (bhash=borders[i];bhash;bhash=bhash->nexthash) {
      border * b;
      for (b=bhash;b!=NULL;b=b->next) {
        if (b->type->valid && !b->type->valid(b)) continue;
        fprintf(f, "%s %d %d %d %d %d ", b->type->__name, b->id, b->from->x, b->from->y, b->to->x, b->to->y);
        if (b->type->write) b->type->write(b, f);
        a_write(f, b->attribs);
        putc('\n', f);
      }
    }
  }
  fputs("end ", f);
}

int
read_borders(FILE * f)
{
  for (;;) {
    short fx, fy, tx, ty;
    unsigned int bid = 0;
    char zText[32];
    border * b;
    region * from, * to;
    border_type * type;
    int result;

    result = fscanf(f, "%s", zText);
    if (result<0) return result;
    if (!strcmp(zText, "end")) break;
    result = fscanf(f, "%u %hd %hd %hd %hd", &bid, &fx, &fy, &tx, &ty);
    if (result<0) return result;

    from = findregion(fx, fy);
    if (!incomplete_data && from==NULL) {
      log_error(("border for unknown region %d,%d\n", fx, fy));
    }
    to = findregion(tx, ty);
    if (!incomplete_data && to==NULL)  {
      log_error(("border for unknown region %d,%d\n", tx, ty));
    }

    type = find_bordertype(zText);
    if (type==NULL) {
      log_error(("[read_borders] unknown border type %s in %s\n", zText, 
        regionname(from, NULL)));
      assert(type || !"border type not registered");
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
    if (type->read) type->read(b, f);
    result = a_read(f, &b->attribs);
    if (result<0) return result;
    if (!to || !from) {
      erase_border(b);
    }
  }
  return 0;
}
