/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include "eressea.h"
#include "building.h"
#include "attrib.h"

/* kernel includes */
#include "item.h"
#include "curse.h" /* für C_NOCOST */
#include "unit.h"
#include "region.h"
#include "skill.h"
#include "save.h"

/* util includes */
#include <base36.h>
#include <functions.h>
#include <resolve.h>
#include <event.h>
#include <language.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
lc_write(const struct attrib * a, FILE* F)
{
  building_action * data = (building_action*)a->data.v;
  const char * fname = data->fname;
  const char * fparam = data->param;
  building * b = data->b;

  write_building_reference(b, F);
  fwritestr(F, fname);
#if RELEASE_VERSION>=BACTION_VERSION
  fputc(' ', F);
  fwritestr(F, fparam?fparam:NULLSTRING);
#endif
  fputc(' ', F);
}

static int
lc_read(struct attrib * a, FILE* F)
{
  char lbuf[256];
  building_action * data = (building_action*)a->data.v;

  read_building_reference(&data->b, F);
  freadstr(F, lbuf, sizeof(lbuf));
  data->fname = strdup(lbuf);
  if (global.data_version>=BACTION_VERSION) {
    freadstr(F, lbuf, sizeof(lbuf));
    if (strcmp(lbuf, NULLSTRING)==0) data->param = NULL;
    else data->param = strdup(lbuf);
  } else {
    data->param = strdup(NULLSTRING);
  }
  return AT_READ_OK;
}

attrib_type at_building_action = {
  "lcbuilding", 
  lc_init, lc_done, 
  NULL, 
  lc_write, lc_read
};

attrib_type at_nodestroy = {
	"nodestroy",
	NULL, NULL, NULL,
	a_writedefault, 
	a_readdefault,
	ATF_UNIQUE
};

building_typelist *buildingtypes;

const building_type *
bt_find(const char* name)
{
	const struct building_typelist * btl = buildingtypes;

	if (global.data_version < RELEASE_VERSION) {
		const char * translation[3][2] = { 
			{ "illusion", "illusioncastle" }, 
			{ "generic", "genericbuilding" }, 
			{ NULL, NULL } 
		};
		int i;
		for (i=0;translation[i][0];++i) {
			/* calling a building "illusion" was a bad idea" */
			if (strcmp(translation[i][0], name)==0) {
				name = translation[i][1];
				break;
			}
		}
	}
	while (btl && strcasecmp(btl->type->_name, name)) btl = btl->next;
	if (!btl) {
		btl = buildingtypes;
		while (btl && strncasecmp(btl->type->_name, name, strlen(name))) btl = btl->next;
	}
	return btl?btl->type:NULL;
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
      return min(b->type->maxcapacity, b->size * b->type->capacity);
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
buildingtype(const building * b, int bsize)
{
	const char * s = NULL;
	const building_type * btype = b->type;
	static const struct building_type * bt_generic;
	if (!bt_generic) bt_generic = bt_find("generic");
	assert(bt_generic);

	if (btype == bt_generic) {
		const attrib *a = a_find(b->attribs, &at_building_generic_type);
		if (a) s = (const char*)a->data.v;
	}

	if (btype->name) s = btype->name(bsize);
	if (s==NULL) s = btype->_name;
	return s;
}

int
buildingmaintenance(const building * b, const resource_type * rtype)
{
  const building_type * bt = b->type;
  int c, cost=0;
  static boolean init = false;
  static const curse_type * nocost_ct;
  if (!init) { init = true; nocost_ct = ct_find("nocost"); }
  if (curse_active(get_curse(b->attribs, nocost_ct))) {
    return 0;
  }
  for (c=0;bt->maintenance && bt->maintenance[c].number;++c) {
    const maintenance * m = bt->maintenance + c;
    if (m->rtype==rtype) {
      if (fval(m, MTF_VARIABLE))
        cost += (b->size * m->number);
      else
        cost += m->number;
    }
  }
  return cost;
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
#if LARGE_CASTLES
	B_TRADEPOST,
#endif
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
castle_name(int bsize)
{
	const char * fname[MAXBUILDINGS] = {
	  "site",
#if LARGE_CASTLES
		"tradepost",
#endif
	  "fortification",
	  "tower",
	  "castle",
	  "fortress",
	  "citadel" };
	const construction * ctype;
	static const struct building_type * bt_castle;
	int i = 0;

	if (!bt_castle) bt_castle = bt_find("castle");
	assert(bt_castle);
	ctype = bt_castle->construction;
	while (ctype &&
			 ctype->maxsize != -1
			 && ctype->maxsize<=bsize) {
		bsize-=ctype->maxsize;
		ctype=ctype->improvement;
		++i;
	}
	return fname[i];
}

static requirement castle_req[] = {
	{ R_STONE, 1, 0.5 },
	{ NORESOURCE, 0, 0.0 },
};

#if LARGE_CASTLES
static construction castle_bld[MAXBUILDINGS] = {
	{ SK_BUILDING, 1,     2, 1, castle_req, &castle_bld[1] },
	{ SK_BUILDING, 1,     8, 1, castle_req, &castle_bld[2] },
	{ SK_BUILDING, 2,    40, 1, castle_req, &castle_bld[3] },
	{ SK_BUILDING, 3,   200, 1, castle_req, &castle_bld[4] },
	{ SK_BUILDING, 4,  1000, 1, castle_req, &castle_bld[5] },
	{ SK_BUILDING, 5,  5000, 1, castle_req, &castle_bld[6] },
	{ SK_BUILDING, 6,    -1, 1, castle_req, NULL }
};
#else
static construction castle_bld[MAXBUILDINGS] = {
	{ SK_BUILDING, 1,    2, 1, castle_req, &castle_bld[1] },
	{ SK_BUILDING, 2,    8, 1, castle_req, &castle_bld[2] },
	{ SK_BUILDING, 3,   40, 1, castle_req, &castle_bld[3] },
	{ SK_BUILDING, 4,  200, 1, castle_req, &castle_bld[4] },
	{ SK_BUILDING, 5, 1000, 1, castle_req, &castle_bld[5] },
	{ SK_BUILDING, 6,   -1, 1, castle_req, NULL }
};
#endif

building_type bt_castle = {
	"castle",
	BFL_NONE,
	1, 4, -1,
	0, 0, 0, 0.0,
	NULL,
	&castle_bld[0],
	castle_name
};

/* for finding out what was meant by a particular building string */

static local_names * bnames;

const building_type *
findbuildingtype(const char * name, const struct locale * lang)
{
	local_names * bn = bnames;
	void * i;

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
			addtoken(&bn->names, n, (void*)btl->type);
			btl=btl->next;
		}
		bnames = bn;
	}
	if (findtoken(&bn->names, name, &i)==E_TOK_NOMATCH) return NULL;
	return (const building_type*)i;
}


void
register_buildings(void)
{
	register_function((pf_generic)init_smithy, "init_smithy");
	register_function((pf_generic)castle_name, "castle_name");
	bt_register(&bt_castle);
}

building_type *
bt_make(const char * name, int flags, int capacity, int maxcapacity, int maxsize)
{
	building_type * btype = calloc(sizeof(building_type), 1);
	btype->_name = name;
	btype->flags = flags | BTF_DYNAMIC;
	btype->capacity = capacity;
	btype->maxcapacity = maxcapacity;
	btype->maxsize = maxsize;

	bt_register(btype);
	return btype;
}

void *
resolve_building(void * id) {
   return findbuilding((int)id);
}

void
write_building_reference(const struct building * b, FILE * F)
{
	fprintf(F, "%s ", b?itoa36(b->no):"0");
}

int
read_building_reference(struct building ** b, FILE * F)
{
	int id;
	char zText[10];
	fscanf(F, "%s ", zText);
	id = atoi36(zText);
	if (id==0) {
		*b = NULL;
		return AT_READ_FAIL;
	}
	else {
		*b = findbuilding(id);
		if (*b==NULL) ur_add((void*)id, (void**)b, resolve_building);
		return AT_READ_OK;
	}
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

	b->no  = newcontainerid();
	bhash(b);

	b->type = btype;
	set_string(&b->display, "");
	b->region = r;
	addlist(&r->buildings, b);

	{
		static char buffer[IDSIZE + 1 + NAMESIZE + 1];
		if (b->type->name)
			sprintf(buffer, "%s", locale_string(lang, btype->_name));
		else
			sprintf(buffer, "%s", LOC(lang, buildingtype(b, 0)));
		set_string(&b->name, buffer);
	}
	return b;
}

void
destroy_building(building * b)
{
	unit *u;
  direction_t d;
  static const struct building_type * bt_caravan, * bt_dam, * bt_tunnel;
  boolean init = false;

  if (!init) {
    init = true;
    bt_caravan = bt_find("caravan");
    bt_dam = bt_find("dam");
    bt_tunnel = bt_find("tunnel");
  }

	if (!bfindhash(b->no)) return;
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

#if 0	/* Memoryleak. Aber ohne klappt das Rendern nicht! */
	removelist(&b->region->buildings, b);
#endif
	/* Stattdessen nur aus Liste entfernen, aber im Speicher halten. */
	choplist(&b->region->buildings, b);
	handle_event(&b->attribs, "destroy", b);
}

extern attrib_type at_icastle;

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

#ifdef BETA_CODE
void
xml_writebuildings(void)
{
	FILE * F = fopen("buildings.xml", "w");
	building_typelist *btl= buildingtypes;
	while (btl) {
		int i;
		const building_type * bt = btl->type;
		fprintf(F, "<building name=\"%s\"", bt->_name);
		if (bt->capacity>=0) fprintf(F, " capacity=\"%d\"", bt->capacity);
		if (bt->maxcapacity>=0) fprintf(F, " maxcapacity=\"%d\"", bt->maxcapacity);
		if (bt->maxsize>=0) fprintf(F, " maxsize=\"%d\"", bt->maxsize);
		if (bt->flags & BTF_INDESTRUCTIBLE) fputs(" nodestroy", F);
		if (bt->flags & BTF_NOBUILD) fputs(" nobuild", F);
		if (bt->flags & BTF_UNIQUE) fputs(" unique", F);
		if (bt->flags & BTF_DECAY) fputs(" decay", F);
		if (bt->flags & BTF_PROTECTION) fputs(" protection", F);
		if (bt->flags & BTF_MAGIC) fputs(" magic", F);
		fputs(">\n", F);
		if (bt->name) {
			const char * name = get_functionname((pf_generic)bt->name);
			assert(name);
			fprintf(F, "\t<function name=\"name\" value=\"%s\"></function>\n",
				name);
		}
		if (bt->init) {
			const char * name = get_functionname((pf_generic)bt->init);
			assert(name);
			fprintf(F, "\t<function name=\"init\" value=\"%s\"></function>\n",
				name);
		}
		for (i=0;bt->maintenance && bt->maintenance[i].number;++i) {
			const maintenance * m = bt->maintenance + i;
			fprintf(F, "\t<maintenance type=\"%s\" amount=\"%u\"", 
				oldresourcetype[m->type]->_name[0], m->number);
			if (m->flags & MTF_VARIABLE) fputs(" variable", F);
			if (m->flags & MTF_VITAL) fputs(" vital", F);
			fputs(">\n", F);
		}
		if (bt->construction) {
			fprintf(F, "\t<construction skill=\"%s\" minskill=\"%u\" reqsize=\"%u\"", 
				skillname(bt->construction->skill, NULL), bt->construction->minskill, 
				bt->construction->reqsize);
			if (bt->construction->maxsize>=0) fprintf(F, " maxsize=\"%d\"", bt->construction->maxsize);
			fputs(">\n", F);
			for (i=0;bt->construction->materials[i].number!=0;++i) {
				fprintf(F, "\t\t<requirement type=\"%s\" quantity=\"%d\"></requirement>\n", 
					oldresourcetype[bt->construction->materials[i].type]->_name[0], 
					bt->construction->materials[i].number);
			}
			fputs("\t</construction>\n", F);
		}
		fputs("</building>\n\n", F);
		btl=btl->next;
	}
	fclose(F);
}

#endif
