/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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

#include <kernel/eressea.h>
#include "building.h"

/* kernel includes */
#include "item.h"
#include "curse.h" /* für C_NOCOST */
#include "unit.h"
#include "region.h"
#include "skill.h"
#include "magic.h"
#include "save.h"
#include "version.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/storage.h>
#include <util/umlaut.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* attributes includes */
#include <attributes/matmod.h>

static const char * NULLSTRING = "(null)";

static void 
lc_init(struct attrib *a)
{
  a->data.v = calloc(1, sizeof(building_action));
}

static void 
lc_done(struct attrib *a)
{
  building_action * data = (building_action*)a->data.v;
  if (data->fname) free(data->fname);
  if (data->param) free(data->param);
  free(data);
}

static void 
lc_write(const struct attrib * a, struct storage * store)
{
  building_action * data = (building_action*)a->data.v;
  const char * fname = data->fname;
  const char * fparam = data->param;
  building * b = data->b;

  write_building_reference(b, store);
  store->w_tok(store, fname);
  store->w_tok(store, fparam?fparam:NULLSTRING);
}

static int
lc_read(struct attrib * a, struct storage * store)
{
  building_action * data = (building_action*)a->data.v;
  int result = read_reference(&data->b, store, read_building_reference, resolve_building);
  if (store->version<UNICODE_VERSION) {
    data->fname = store->r_str(store);
  } else {
    data->fname = store->r_tok(store);
  }
  if (store->version>=BACTION_VERSION) {
    char lbuf[256];
    if (store->version<UNICODE_VERSION) {
      store->r_str_buf(store, lbuf, sizeof(lbuf));
    } else {
      store->r_tok_buf(store, lbuf, sizeof(lbuf));
    }
    if (strcmp(lbuf, NULLSTRING)==0) data->param = NULL;
    else data->param = strdup(lbuf);
  } else {
    data->param = strdup(NULLSTRING);
  }
  if (result==0 && !data->b) {
    return AT_READ_FAIL;
  }
  return AT_READ_OK;
}

attrib_type at_building_action = {
  "lcbuilding", 
  lc_init, lc_done, 
  NULL, 
  lc_write, lc_read
};

typedef struct building_typelist {
  struct building_typelist * next;
  building_type * type;
} building_typelist;

static building_typelist *buildingtypes;

building_type *
bt_find(const char* name)
{
  const struct building_typelist * btl = buildingtypes;
  while (btl && strcmp(btl->type->_name, name)) btl = btl->next;
  if (btl==NULL) {
    return NULL;
  }
  return btl->type;
}

void
bt_register(building_type * type)
{
	struct building_typelist * btl = malloc(sizeof(building_type));
	if (type->init) type->init(type);
	btl->type = type;
	btl->next = buildingtypes;
	buildingtypes = btl;
}

int
buildingcapacity(const building * b)
{
  if (b->type->capacity>=0) {
    if (b->type->maxcapacity>=0) {
      return MIN(b->type->maxcapacity, b->size * b->type->capacity);
    }
    return b->size * b->type->capacity;
  }
  if (b->size>=b->type->maxsize) {
    if (b->type->maxcapacity>=0) {
      return b->type->maxcapacity;
    }
  }
  return 0;
}

attrib_type at_building_generic_type = {
	  "building_generic_type", NULL, NULL, NULL, a_writestring, a_readstring, ATF_UNIQUE
};

const char *
buildingtype(const building_type * btype, const building * b, int bsize)
{
	const char * s = NULL;
  static boolean init_generic = false;
	static const struct building_type * bt_generic;

  if (!init_generic) {
    init_generic = true;
    bt_generic = bt_find("generic");
  }

  if (btype == bt_generic) {
		const attrib *a = a_find(b->attribs, &at_building_generic_type);
		if (a) s = (const char*)a->data.v;
	}

	if (btype->name) s = btype->name(btype, bsize);
	if (s==NULL) s = btype->_name;
	return s;
}

#define BMAXHASH 7919
static building *buildhash[BMAXHASH];
void
bhash(building * b)
{
  building *old = buildhash[b->no % BMAXHASH];

  buildhash[b->no % BMAXHASH] = b;
  b->nexthash = old;
}

void
bunhash(building * b)
{
  building **show;

  for (show = &buildhash[b->no % BMAXHASH]; *show; show = &(*show)->nexthash) {
    if ((*show)->no == b->no)
      break;
  }
  if (*show) {
    assert(*show == b);
    *show = (*show)->nexthash;
    b->nexthash = 0;
  }
}

static building *
bfindhash(int i)
{
	building *old;

	for (old = buildhash[i % BMAXHASH]; old; old = old->nexthash)
		if (old->no == i)
			return old;
	return 0;
}

building *
findbuilding(int i)
{
	return bfindhash(i);
}
/* ** old building types ** */


/** Building: Fortification */
enum {
	B_SITE,
	B_TRADEPOST,
	B_FORTIFICATION,
	B_TOWER,
	B_CASTLE,
	B_FORTRESS,
	B_CITADEL,
	MAXBUILDINGS
};

static int
sm_smithy(const unit * u, const region * r, skill_t sk, int value) /* skillmod */
{
	if (sk==SK_WEAPONSMITH || sk==SK_ARMORER) {
		if (u->region == r) return value + 1;
	}
	return value;
}
static int
mm_smithy(const unit * u, const resource_type * rtype, int value) /* material-mod */
{
	if (rtype == oldresourcetype[R_IRON]) return value * 2;
	return value;
}
static void
init_smithy(struct building_type * bt)
{
	a_add(&bt->attribs, make_skillmod(NOSKILL, SMF_PRODUCTION, sm_smithy, 1.0, 0));
	a_add(&bt->attribs, make_matmod(mm_smithy));
}

static const char *
castle_name(const struct building_type* btype, int bsize)
{
	const char * fname[MAXBUILDINGS] = {
	  "site",
		"tradepost",
	  "fortification",
	  "tower",
	  "castle",
	  "fortress",
	  "citadel" };
	const construction * ctype;
	int i = 0;

	ctype = btype->construction;
	while (ctype && ctype->maxsize != -1 && ctype->maxsize<=bsize) {
		bsize-=ctype->maxsize;
		ctype=ctype->improvement;
		++i;
	}
	return fname[i];
}

#ifdef WDW_PYRAMID

static const char *
pyramid_name(const struct building_type* btype, int bsize)
{
  static char p_name_buf[32];
  int level=0;
  const construction * ctype;

  ctype = btype->construction;
  
	while (ctype && ctype->maxsize != -1 && ctype->maxsize<=bsize) {
    bsize-=ctype->maxsize;
    ctype=ctype->improvement;
    ++level;
  }

  sprintf(p_name_buf, "pyramid%d", level);

  return p_name_buf;
}

int
wdw_pyramid_level(const struct building *b)
{
  const construction *ctype = b->type->construction;
  int completed = b->size;
  int level = 0;

  while(ctype->improvement != NULL &&
      ctype->improvement != ctype &&
      ctype->maxsize > 0 &&
      ctype->maxsize <= completed)
  {
    ++level;
    completed-=ctype->maxsize;
    ctype = ctype->improvement;
  }

  return level;
}
#endif

/* for finding out what was meant by a particular building string */

static local_names * bnames;

const building_type *
findbuildingtype(const char * name, const struct locale * lang)
{
  variant type;
	local_names * bn = bnames;

	while (bn) {
		if (bn->lang==lang) break;
		bn=bn->next;
	}
	if (!bn) {
		struct building_typelist * btl = buildingtypes;
		bn = calloc(sizeof(local_names), 1);
		bn->next = bnames;
		bn->lang = lang;
		while (btl) {
			const char * n = locale_string(lang, btl->type->_name);
      type.v = (void*)btl->type;
			addtoken(&bn->names, n, type);
			btl=btl->next;
		}
		bnames = bn;
	}
	if (findtoken(&bn->names, name, &type)==E_TOK_NOMATCH) return NULL;
	return (const building_type*)type.v;
}


void
register_buildings(void)
{
	register_function((pf_generic)init_smithy, "init_smithy");
	register_function((pf_generic)castle_name, "castle_name");
#ifdef WDW_PYRAMID
	register_function((pf_generic)pyramid_name, "pyramid_name");
#endif
}

void
write_building_reference(const struct building * b, struct storage * store)
{
  store->w_id(store, (b && b->region)?b->no:0);
}


int
resolve_building(variant id, void * address)
{
  building * b = NULL;
  if (id.i!=0) {
    b = findbuilding(id.i);
    if (b==NULL) {
      return -1;
    }
  }
  *(building**)address = b;
  return 0;
}

variant
read_building_reference(struct storage * store)
{
  variant result;
  result.i = store->r_id(store);
  return result;
}

void
free_buildinglist(building_list *blist)
{
  while (blist) {
    building_list * rl2 = blist->next;
    free(blist);
    blist = rl2;
  }
}

void
add_buildinglist(building_list **blist, building *b)
{
  building_list *rl2 = (building_list*)malloc(sizeof(building_list));

  rl2->data = b;
  rl2->next = *blist;

  *blist = rl2;
}

building *
new_building(const struct building_type * btype, region * r, const struct locale * lang)
{
  building *b = (building *) calloc(1, sizeof(building));
  static boolean init_lighthouse = false;
  static const struct building_type * bt_lighthouse = 0;

  if (!init_lighthouse) {
    bt_lighthouse = bt_find("lighthouse");
    init_lighthouse = true;
  }

  b->no  = newcontainerid();
  bhash(b);
  
  b->type = btype;
  b->region = r;
  addlist(&r->buildings, b);
  
  if (b->type==bt_lighthouse) {
    r->flags |= RF_LIGHTHOUSE;
  }
  {
    const char * bname;
    if (b->type->name==NULL) {
      bname = LOC(lang, btype->_name);
    } else {
      bname = LOC(lang, buildingtype(btype, b, 0));
    }
    b->name = strdup(bname);
  }
  return b;
}

static building * deleted_buildings;

/** remove a building from the region.
 * remove_building lets units leave the building
 */
void
remove_building(building ** blist, building * b)
{
  unit *u;
  direction_t d;
  static const struct building_type * bt_caravan, * bt_dam, * bt_tunnel;
  static boolean init = false;

  if (!init) {
    init = true;
    bt_caravan = bt_find("caravan");
    bt_dam = bt_find("dam");
    bt_tunnel = bt_find("tunnel");
  }

  assert(bfindhash(b->no));

  handle_event(b->attribs, "destroy", b);
  for (u=b->region->units; u; u=u->next) {
    if (u->building == b) leave(b->region, u);
  }
  
  b->size = 0;
  update_lighthouse(b);
  bunhash(b);
  
  /* Falls Karawanserei, Damm oder Tunnel einstürzen, wird die schon
   * gebaute Straße zur Hälfte vernichtet */
  if (b->type == bt_caravan || b->type == bt_dam || b->type == bt_tunnel) {
    region * r = b->region;
    for (d=0;d!=MAXDIRECTIONS;++d) if (rroad(r, d) > 0) {
      rsetroad(r, d, rroad(r, d) / 2);
    }
  }

  /* Stattdessen nur aus Liste entfernen, aber im Speicher halten. */
  while (*blist && *blist!=b) blist = &(*blist)->next;
  *blist = b->next;
  b->region = NULL;
  b->next = deleted_buildings;
  deleted_buildings = b;
}

void
free_building(building * b)
{
  while (b->attribs) a_remove (&b->attribs, b->attribs);
  free(b->name);
  free(b->display);
  free(b);
}

void
free_buildings(void)
{
  while (deleted_buildings) {
    building * b = deleted_buildings;
    deleted_buildings = b->next;
  }
}

extern struct attrib_type at_icastle;

/** returns the building's build stage (NOT size in people).
 * only makes sense for castles or similar buildings with multiple
 * stages */
int
buildingeffsize(const building * b, boolean img)
{
  int i = b->size, n = 0;
  const construction * cons;
  static const struct building_type * bt_castle;
  if (!bt_castle) bt_castle = bt_find("castle");
  assert(bt_castle);

  if (b==NULL) return 0;

  if (b->type!=bt_castle) {
    if (img) {
      const attrib * a = a_find(b->attribs, &at_icastle);
      if (!a || a->data.v != bt_castle) return 0;
    } else return 0;
  }
  cons = bt_castle->construction;
  assert(cons);

  while (cons && cons->maxsize != -1 && i>=cons->maxsize) {
    i-=cons->maxsize;
    cons = cons->improvement;
    ++n;
  }

  return n;
}

const char *
write_buildingname(const building * b, char * ibuf, size_t size)
{
  snprintf((char*)ibuf, size, "%s (%s)", b->name, itoa36(b->no));
  ibuf[size-1] = 0;
  return ibuf;
}

const char *
buildingname(const building * b)
{
  typedef char name[OBJECTIDSIZE + 1];
  static name idbuf[8];
  static int nextbuf = 0;
  char *ibuf = idbuf[(++nextbuf) % 8];
  return write_buildingname(b, ibuf, sizeof(name));
}

unit *
buildingowner(const region * r, const building * b)
{
	unit *u = NULL;
	unit *first = NULL;
#ifndef BROKEN_OWNERS
	assert(r == b->region);
#endif
	/* Prüfen ob Eigentümer am leben. */

	for (u = r->units; u; u = u->next) {
		if (u->building == b) {
			if (!first && u->number > 0)
				first = u;
			if (fval(u, UFL_OWNER) && u->number > 0)
				return u;
			if (u->number == 0)
				freset(u, UFL_OWNER);
		}
	}

	/* Eigentümer tot oder kein Eigentümer vorhanden. Erste lebende Einheit
	 * nehmen. */

	if (first)
		fset(first, UFL_OWNER);
	return first;
}

const char * building_getname(const building * self)
{
  return self->name;
}

void building_setname(building * self, const char * name)
{
  free(self->name);
  if (name) self->name = strdup(name);
  else self->name = NULL;
}

void
building_addaction(building * b, const char * fname, const char * param)
{
  attrib * a = a_add(&b->attribs, a_new(&at_building_action));
  building_action * data = (building_action*)a->data.v;
  data->b = b;
  data->fname = strdup(fname);
  if (param) {
    data->param = strdup(param);
  }
}

region *
building_getregion(const building * b)
{
  return b->region;
}

void
building_setregion(building * b, region * r)
{
  building ** blist = &b->region->buildings;
  while (*blist && *blist!=b) {
    blist = &(*blist)->next;
  }
  *blist = b->next;
  b->next = NULL;

  blist = &r->buildings;
  while (*blist && *blist!=b) blist = &(*blist)->next;
  *blist = b;

  b->region = r;
}

