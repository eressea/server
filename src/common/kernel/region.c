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
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "eressea.h"
#include "region.h"

/* kernel includes */
#include "border.h"
#include "building.h"
#include "curse.h"
#include "faction.h"
#include "item.h"
#include "message.h"
#include "plane.h"
#include "region.h"
#include "resources.h"
#include "unit.h"

/* util includes */
#include <resolve.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int dice_rand(const char *s);

static int g_maxluxuries = 0;

const short delta_x[MAXDIRECTIONS] =
{
  -1, 0, 1, 1, 0, -1
};

const short delta_y[MAXDIRECTIONS] =
{
  1, 1, 0, -1, -1, 0
};

static const direction_t back[MAXDIRECTIONS] =
{
  D_SOUTHEAST,
    D_SOUTHWEST,
    D_WEST,
    D_NORTHWEST,
    D_NORTHEAST,
    D_EAST,
};

direction_t 
dir_invert(direction_t dir)
{
  switch (dir) {
    case D_PAUSE:
    case D_SPECIAL:
      return dir;
      break;
    default:
      if (dir>=0 && dir<MAXDIRECTIONS) return back[dir];
  }
  assert(!"illegal direction");
  return NODIRECTION;
}
const char *
regionname(const region * r, const faction * f)
{
	static char buf[65];
  const struct locale * lang = f ? f->locale : 0;
	plane *pl = getplane(r);
	if (r==NULL) {
		strcpy(buf, "(null)");
	} else if (pl && fval(pl, PFL_NOCOORDS)) {
		strncpy(buf, rname(r, lang), 65);
	} else {
#ifdef HAVE_SNPRINTF
		snprintf(buf, 65, "%s (%d,%d)", rname(r, lang), 
             region_x(r, f), region_y(r, f));
#else
		strncpy(buf, rname(r, lang), 50);
		buf[50]=0;
		sprintf(buf+strlen(buf), " (%d,%d)", region_x(r, f), region_y(r, f));
#endif
	}
	buf[64] = 0;
	return buf;
}

int
deathcount(const region * r) {
	attrib * a = a_find(r->attribs, &at_deathcount);
	if (!a) return 0;
	return a->data.i;
}

int
chaoscount(const region * r) {
	attrib * a = a_find(r->attribs, &at_chaoscount);
	if (!a) return 0;
	return a->data.i;
}

int
woodcount(const region * r) {
	attrib * a = a_find(r->attribs, &at_woodcount);
	if (!a) return 0;
	return a->data.i;
}

void
deathcounts (region * r, int fallen) {
	attrib * a;

	if (fallen==0) return;
	if (is_cursed(r->attribs, C_HOLYGROUND,0)) return;

	a = a_find(r->attribs, &at_deathcount);
	if (!a) a = a_add(&r->attribs, a_new(&at_deathcount));
	a->data.i += fallen;

	if (a->data.i<=0) a_remove(&r->attribs, a);
}

void
chaoscounts(region * r, int fallen) {
	attrib * a;

	if (fallen==0) return;

	a = a_find(r->attribs, &at_chaoscount);
	if (!a) a = a_add(&r->attribs, a_new(&at_chaoscount));
	a->data.i += fallen;

	if (a->data.i<=0) a_remove(&r->attribs, a);
}

void
woodcounts(region * r, int fallen) {
	attrib * a;

	if (fallen==0) return;

	a = a_find(r->attribs, &at_woodcount);
	if (!a) a = a_add(&r->attribs, a_new(&at_woodcount));
	a->data.i += fallen;

	if (a->data.i<=0) a_remove(&r->attribs, a);
}

/********************/
/*   at_direction   */
/********************/
void
a_initdirection(attrib *a)
{
	a->data.v = calloc(1, sizeof(spec_direction));
}

int
a_agedirection(attrib *a)
{
	spec_direction *d = (spec_direction *)(a->data.v);

	if (d->duration > 0) d->duration--;
	else d->duration = 0;

	return d->duration;
}

int
a_readdirection(attrib *a, FILE *f)
{
	spec_direction *d = (spec_direction *)(a->data.v);
	fscanf(f, "%hd %hd %d", &d->x, &d->y, &d->duration);
	fscanf(f, "%s ", buf);
	d->desc = strdup(cstring(buf));
	fscanf(f, "%s ", buf);
	d->keyword = strdup(cstring(buf));
        d->active = true;
	return AT_READ_OK;
}

void
a_writedirection(const attrib *a, FILE *f)
{
	spec_direction *d = (spec_direction *)(a->data.v);

	fprintf(f, "%d %d %d %s %s ", d->x, d->y, d->duration,
			estring(d->desc), estring(d->keyword));
}

attrib_type at_direction = {
	"direction",
	a_initdirection,
	NULL,
	a_agedirection,
	a_writedirection,
	a_readdirection
};


attrib *
create_special_direction(region *r, region * rt, int duration,
                         const char *desc, const char *keyword)

{
  attrib *a = a_add(&r->attribs, a_new(&at_direction));
  spec_direction *d = (spec_direction *)(a->data.v);

  d->active = false;
  d->x = rt->x;
  d->y = rt->y;
  d->duration = duration;
  d->desc = strdup(desc);
  d->keyword = strdup(keyword);

  return a;
}

/* Moveblock wird zur Zeit nicht über Attribute, sondern ein Bitfeld
   r->moveblock gemacht. Sollte umgestellt werden, wenn kompliziertere
	 Dinge gefragt werden. */

/********************/
/*   at_moveblock   */
/********************/
void
a_initmoveblock(attrib *a)
{
	a->data.v = calloc(1, sizeof(moveblock));
}

int
a_readmoveblock(attrib *a, FILE *f)
{
	moveblock *m = (moveblock *)(a->data.v);
	int       i;

	fscanf(f, "%d", &i);
	m->dir = (direction_t)i;
	return AT_READ_OK;
}

void
a_writemoveblock(const attrib *a, FILE *f)
{
	moveblock *m = (moveblock *)(a->data.v);
	fprintf(f, "%d ", (int)m->dir);
}

attrib_type at_moveblock = {
	"moveblock", a_initmoveblock, NULL, NULL, a_writemoveblock, a_readmoveblock
};


#define RMAXHASH 65521
region *regionhash[RMAXHASH];

static region *
rfindhash(short x, short y)
{
	region *old;
	for (old = regionhash[coor_hashkey(x, y) % RMAXHASH]; old; old = old->nexthash)
		if (old->x == x && old->y == y)
			return old;
	return 0;
}

void
rhash(region * r)
{
  int key = coor_hashkey(r->x, r->y) % RMAXHASH;
	region *old = regionhash[key];
	regionhash[key] = r;
	r->nexthash = old;
}

void
runhash(region * r)
{
	region **show;
	for (show = &regionhash[coor_hashkey(r->x, r->y) % RMAXHASH]; *show; show = &(*show)->nexthash) {
		if ((*show)->x == r->x && (*show)->y == r->y)
			break;
	}
	if (*show) {
		assert(*show == r);
		*show = (*show)->nexthash;
		r->nexthash = 0;
	}
}

region *
r_connect(const region * r, direction_t dir)
{
	static int set = 0;
	static region * buffer[MAXDIRECTIONS];
	static const region * last = NULL;

#ifdef FAST_CONNECT
	region * rmodify = (region*)r;
        assert (dir>=0 && dir<MAXDIRECTIONS);
	if (r->connect[dir]) return r->connect[dir];
#endif
	assert(dir<MAXDIRECTIONS);
	if (r != last) {
		set = 0;
		last = r;
	}
	else
		if (set & (1 << dir)) return buffer[dir];
	buffer[dir] = rfindhash(r->x + delta_x[dir], r->y + delta_y[dir]);
	set |= (1<<dir);
#ifdef FAST_CONNECT
	rmodify->connect[dir] = buffer[dir];
#endif
	return buffer[dir];
}

region *
findregion(short x, short y)
{
	return rfindhash(x, y);
}

int
koor_distance(int x1, int y1, int x2, int y2)
{
	/* Contributed by Hubert Mackenberg. Thanks.
	 * x und y Abstand zwischen x1 und x2 berechnen
	 */
	int dx = x1 - x2;
	int dy = y1 - y2;

	/* Bei negativem dy am Ursprung spiegeln, das veraendert
	 * den Abstand nicht
	 */
	if ( dy < 0 )
	{
		dy = -dy;
		dx = -dx;
	}

	/*
	 * dy ist jetzt >=0, fuer dx sind 3 Faelle zu untescheiden
	 */
	if      ( dx >= 0 ) return dx + dy;
	else if (-dx >= dy) return -dx;
	else                return dy;
}

int
distance(const region * r1, const region * r2)
{
	return koor_distance(r1->x, r1->y, r2->x, r2->y);
}

static direction_t
koor_reldirection(int ax, int ay, int bx, int by)
{
  direction_t dir;
  for (dir=0;dir!=MAXDIRECTIONS;++dir) {
    if (bx-ax == delta_x[dir] && by-ay == delta_y[dir]) return dir;
  }
  return NODIRECTION;
}

spec_direction *
special_direction(const region * from, const region * to)
{
  const attrib *a = a_findc(from->attribs, &at_direction);
  
  while (a!=NULL) {
    spec_direction * sd = (spec_direction *)a->data.v;
    if (sd->x==to->x && sd->y==to->y) return sd;
    a = a->nexttype;
  }
  return NULL;
}

direction_t
reldirection(const region * from, const region * to)
{
  direction_t dir = koor_reldirection(from->x, from->y, to->x, to->y);

  if (dir==NODIRECTION) {
    spec_direction *sd = special_direction(from, to);
    if (sd!=NULL && sd->active) return D_SPECIAL;
  }
  return dir;
}

void
free_regionlist(region_list *rl)
{
	while (rl) {
		region_list * rl2 = rl->next;
		free(rl);
		rl = rl2;
	}
}

void
add_regionlist(region_list **rl, region *r)
{
	region_list *rl2 = (region_list*)malloc(sizeof(region_list));

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
	ATF_UNIQUE
};

/*********************/
/*   at_chaoscount   */
/*********************/
attrib_type at_chaoscount = {
	"chaoscount",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	DEFAULT_WRITE,
	DEFAULT_READ,
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
	DEFAULT_WRITE,
	DEFAULT_READ,
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
	DEFAULT_WRITE,
	DEFAULT_READ,
	ATF_UNIQUE
};

/*********************/
/*   at_travelunit   */
/*********************/
attrib_type at_travelunit = {
	"travelunit",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	NO_WRITE,
	NO_READ
};

extern int laen_read(attrib * a, FILE * F);
#define LAEN_READ laen_read
#define LAEN_WRITE NULL

/***************/
/*   at_laen   */
/***************/
attrib_type at_laen = {
	"laen",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	LAEN_WRITE,
	LAEN_READ,
	ATF_UNIQUE
};

void
rsetlaen(region * r, int val)
{
	attrib * a = a_find(r->attribs, &at_laen);
	if (!a && val>=0) a = a_add(&r->attribs, a_new(&at_laen));
	else if (a && val<0) a_remove(&r->attribs, a);
	if (val>=0) a->data.i = val;
}

int
rlaen(const region * r)
{
	attrib * a = a_find(r->attribs, &at_laen);
	if (!a) return -1;
	return a->data.i;
}

/***************/
/*   at_road   */
/***************/
attrib_type at_road = {
	"road", 
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
#if RELEASE_VERSION<NEWROAD_VERSION
	DEFAULT_WRITE,
#else
	NULL,
#endif
	DEFAULT_READ
};

void
rsetroad(region * r, direction_t d, short val)
{
	border * b;
	region * r2 = rconnect(r, d);

	if (!r2) return;
	b = get_borders(r, r2);
	while (b && b->type!=&bt_road) b = b->next;
	if (!b) b = new_border(&bt_road, r, r2);
	if (r==b->from) b->data.sa[0] = val;
	else b->data.sa[1] = val;
}

short
rroad(const region * r, direction_t d)
{
	int rval;
	border * b;
	region * r2 = rconnect(r, d);

	if (!r2) return 0;
	b = get_borders(r, r2);
	while (b && b->type!=&bt_road) b = b->next;
	if (!b) return 0;
	rval = b->data.i;
  if (r==b->from) return b->data.sa[0];
  return b->data.sa[1];
}

boolean
r_isforest(const region * r)
{
	if (r->terrain==T_PLAIN && rtrees(r,2) + rtrees(r,1) >= 600) return true;
	return false;
}

boolean
r_issea(const region * r)
{
	direction_t d;

	for(d=0; d < MAXDIRECTIONS; d++) {
		region *rc = rconnect(r,d);
		if(rc && rterrain(rc) == T_OCEAN) return false;
	}
	return true;
}

boolean
r_isglacier(const region * r)
{
	if (r->terrain==T_GLACIER || r->terrain==T_ICEBERG_SLEEP) return true;
	return false;
}

int
is_coastregion(region *r)
{
	direction_t i;
	int res = 0;

	for(i=0;i<MAXDIRECTIONS;i++) {
		if(rconnect(r,i) && rconnect(r,i)->terrain == T_OCEAN) res++;
	}
	return res;
}

int
rpeasants(const region * r)
{
	return ((r)->land?(r)->land->peasants:0);
}

void
rsetpeasants(region * r, int value)
{
	((r)->land?((r)->land->peasants=(value)):(assert((value)>=0), (value)),0);
}

int
rmoney(const region * r)
{
	return ((r)->land?(r)->land->money:0);
}

void
rsethorses(const region *r, int value)
{
	assert(value >= 0);
	if(r->land)
		r->land->horses = value;
}

int
rhorses(const region *r)
{
	return r->land?r->land->horses:0;
}

void
rsetmoney(region * r, int value)
{
	((r)->land?((r)->land->money=(value)):(assert((value)>=0), (value)),0);
}

void
r_setdemand(region * r, const luxury_type * ltype, int value)
{
	struct demand * d, ** dp = &r->land->demands;
	while (*dp && (*dp)->type != ltype) dp = &(*dp)->next;
	d = *dp;
	if (!d) {
		d = *dp = calloc(sizeof(struct demand), 1);
		d->type = ltype;
	}
	d->value = value;
}

int
r_demand(const region * r, const luxury_type * ltype)
{
	struct demand * d = r->land->demands;
	while (d && d->type != ltype) d = d->next;
	if (!d) return -1;
	return d->value;
}

const char *
rname(const region * r, const struct locale * lang) {
	if (r->land)
		return r->land->name;
	return locale_string(lang, terrain[rterrain(r)].name);
}

int
rtrees(const region *r, int ageclass)
{
	return ((r)->land?(r)->land->trees[ageclass]:0);
}

int
rsettrees(const region *r, int ageclass, int value)
{
	if (!r->land) assert(value==0);
	else {
		assert(value>=0);
		return r->land->trees[ageclass]=value;
	}
	return 0;
}

static region *last;

#ifdef ENUM_REGIONS
static unsigned int max_index = 0;
#endif
region *
new_region(short x, short y)
{
	region *r = rfindhash(x, y);

	if (r) {
		fprintf(stderr, "\ndoppelte regionen entdeckt: %s(%d,%d)\n", regionname(r, NULL), x, y);
		if (r->units)
			fprintf(stderr, "doppelte region enthält einheiten\n");
		return r;
	}
	r = calloc(1, sizeof(region));
	r->x = x;
	r->y = y;
	r->planep = findplane(x, y);
	set_string(&r->display, "");
	rhash(r);
	if (last)
		addlist(&last, r);
	else
		addlist(&regions, r);
	last = r;
#ifdef ENUM_REGIONS
  assert(r->next==NULL);
  r->index = ++max_index;
#endif
	return r;
}

static void
freeland(land_region * lr)
{
	while (lr->demands) {
		struct demand * d = lr->demands;
		lr->demands = d->next;
		free(d);
	}
	if (lr->name) free(lr->name);
	free(lr);
}

void
free_region(region * r)
{
  runhash(r);
  if (last == r) last = NULL;
  free(r->display);
  if (r->land) freeland(r->land);

  if (r->msgs) {
    free_messagelist(r->msgs);
    r->msgs = 0;
  }
  
  while (r->individual_messages) {
    struct individual_message * msg = r->individual_messages;
    r->individual_messages = msg->next;
    if (msg->msgs) free_messagelist(msg->msgs);
    free(msg);
  }

  while (r->attribs) a_remove (&r->attribs, r->attribs);
  while (r->resources) {
    rawmaterial * res = r->resources;
    r->resources = res->next;
    free(res);
  }

  while (r->donations) {
    donation * don = r->donations;
    r->donations = don->next;
    free(don);
  }

  free(r);
}

static char *
makename(void)
{
	int s, v, k, e, p = 0, x = 0;
	size_t nk, ne, nv, ns;
	static char name[16];
	const char *kons = "bcdfghklmnprstvwz",
		*end = "nlrdst",
		*vokal = "aaaaaaaaaàâeeeeeeeeeéèêiiiiiiiiiíîoooooooooóòôuuuuuuuuuúyy",
		*start = "bcdgtskpvfr";

	nk = strlen(kons);
	ne = strlen(end);
	nv = strlen(vokal);
	ns = strlen(start);

	for (s = rand() % 3 + 2; s > 0; s--) {
		if (x > 0) {
			k = rand() % (int)nk;
			name[p] = kons[k];
			p++;
		} else {
			k = rand() % (int)ns;
			name[p] = start[k];
			p++;
		}
		v = rand() % (int)nv;
		name[p] = vokal[v];
		p++;
		if (rand() % 3 == 2 || s == 1) {
			e = rand() % (int)ne;
			name[p] = end[e];
			p++;
			x = 1;
		} else
			x = 0;
	}
	name[p] = '\0';
	name[0] = (char) toupper(name[0]);
	return name;
}

void 
setluxuries(region * r, const luxury_type * sale)
{
	const luxury_type * ltype;

	assert(r->land);

	if(r->land->demands) freelist(r->land->demands);

	for (ltype=luxurytypes; ltype; ltype=ltype->next) {
		struct demand * dmd = calloc(sizeof(struct demand), 1);
		dmd->type = ltype;
		if (ltype!=sale) dmd->value = 1 + rand() % 5;
		dmd->next = r->land->demands;
		r->land->demands = dmd;
	}
}

void
terraform(region * r, terrain_t t)
{
	const struct locale * locale_de = find_locale("de");
	rawmaterial  **lrm;
	int i;

	/* defaults: */
	rsetterrain(r, t);

	/* Resourcen, die nicht mehr vorkommen können, löschen */
	lrm = &r->resources;
	while (*lrm) {
		rawmaterial *rm = *lrm;
		for (i=0; i!=3; ++i) {
			if(terrain[r->terrain].rawmaterials[i].type == rm->type) break;
		}
		if (i==3) {
			*lrm = rm->next;
			free(rm);
		} else {
			lrm = &rm->next;
		}
	}

	if (!landregion(t)) {
		if (r->land) {
			freeland(r->land);
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
		/* Beschreibung und Namen löschen */
		return;
	}

	if (!r->land) {
		static struct surround {
			struct surround * next;
			const luxury_type * type;
			int value;
		} *trash =NULL, *nb = NULL;
		const luxury_type * ltype;
		direction_t d;
		int mnr = 0;

		r->land = calloc(1, sizeof(land_region));
		rsetname(r, makename());
		for (d=0;d!=MAXDIRECTIONS;++d) {
			region * nr = rconnect(r, d);
			if (nr && nr->land) {
				struct demand * sale = r->land->demands;
				while (sale && sale->value!=0) sale=sale->next;
				if (sale) {
					struct surround * sr = nb;
					while (sr && sr->type!=sale->type) sr=sr->next;
					if (!sr) {
						if (trash) {
							sr = trash;
							trash = trash->next;
						} else {
							sr = calloc(1, sizeof(struct surround));
						}
						sr->next = nb;
						sr->type = sale->type;
						sr->value = 1;
						nb = sr;
					} else sr->value++;
					++mnr;
				}
			}
		}
		if (!nb) {
			int i;
			if (g_maxluxuries==0) {
				for (ltype = luxurytypes;ltype;ltype=ltype->next) ++g_maxluxuries;
			}
			i = rand() % g_maxluxuries;
			ltype = luxurytypes;
			while (i--) ltype=ltype->next; 
		} else {
			int i = rand() % mnr;
			struct surround * srd = nb;
			while (i>srd->value) {
				i-=srd->value;
				srd=srd->next;
			}
			setluxuries(r, srd->type);
			while (srd->next!=NULL) srd=srd->next;
			srd->next=trash;
			trash = nb;
			nb = NULL;
		}
	}
	
	if (landregion(t)) {
		const char *name = NULL;
		if (terrain[r->terrain].herbs) {
			int len=0;
			while (terrain[r->terrain].herbs[len]) ++len;
			if (len) name = terrain[r->terrain].herbs[rand()%len];
		}
		if (name != NULL) {
			const item_type * itype = finditemtype(name, locale_de);
			const herb_type * htype = resource2herb(itype->rtype);
			rsetherbtype(r, htype);
			rsetherbs(r, (short)(50+rand()%31));
		}
		else {
			rsetherbtype(r, NULL);
		}
	}

	if (rand() % 100 < ENCCHANCE) {
		fset(r, RF_ENCOUNTER);
	}
	if (rand() % 100 < 3)
		fset(r, RF_MALLORN);
	else
		freset(r, RF_MALLORN);

	switch (t) {
	case T_PLAIN:
		rsethorses(r, rand() % (terrain[t].production_max / 5));
		if(rand()%100 < 40) {
			rsettrees(r, 2, terrain[t].production_max * (30+rand()%40)/100);
			rsettrees(r, 1, rtrees(r, 2)/4);
			rsettrees(r, 0, rtrees(r, 2)/2);
		}
		break;
	case T_MOUNTAIN:
		break;

	case T_GLACIER:
		break;

	case T_ICEBERG_SLEEP:
		/* Kann aus Gletscher entstehen und sollte diesem gleichen */
		break;

	case T_VOLCANO:
	case T_VOLCANO_SMOKING:
		break;
	}

	/* Initialisierung irgendwann über rm_-Mechamismus machen */
	if(t != T_PLAIN && rand()%100 < 20) {
		rsettrees(r, 2, terrain[t].production_max * (30 + rand() % 40) / 100);
		rsettrees(r, 1, rtrees(r, 2)/4);
		rsettrees(r, 0, rtrees(r, 2)/2);
	}

	if (terrain[t].production_max && !fval(r, RF_CHAOTIC)) {
		int peasants;
#if REDUCED_PEASANTGROWTH == 1
		peasants = (maxworkingpeasants(r) * (20+dice_rand("6d10")))/100;
#else
		peasants = MAXPEASANTS_PER_AREA * (rand() % (terrain[t].production_max / 2));
#endif
		rsetpeasants(r, max(100, peasants));
		rsetmoney(r, rpeasants(r) * ((wage(r, NULL, false)+1) + rand() % 5));
	}
}

/** ENNO:
 * ich denke, das das hier nicht sein sollte.
 * statt dessen sollte ein attribut an der region sein, das das erledigt,
 * egal ob durch den spell oder anderes angelegt.
 **/
#include "curse.h"
int
production(const region *r)
{
	/* muß r->terrain sein, nicht rterrain() wegen rekursion */
	int p = terrain[r->terrain].production_max;
	if (curse_active(get_curse(r->attribs, ct_find("drought")))) p /= 2;

	return p;
}

int
read_region_reference(region ** r, FILE * F)
{
	variant coor;
	fscanf(F, "%hd %hd", &coor.sa[0], &coor.sa[1]);
	if (coor.sa[0]==SHRT_MAX) {
		*r = NULL;
		return AT_READ_FAIL;
	}
	*r = findregion(coor.sa[0], coor.sa[1]);

  if (*r==NULL) {
    ur_add(coor, (void**)r, resolve_region);
  }
	return AT_READ_OK;
}

void
write_region_reference(const region * r, FILE * F)
{
	if (r) fprintf(F, "%d %d ", r->x, r->y);
	else fprintf(F, "%d %d ", SHRT_MAX, SHRT_MAX);
}

void *
resolve_region(variant id) {
	return findregion(id.sa[0], id.sa[1]);
}


struct message_list *
r_getmessages(const struct region * r, const struct faction * viewer)
{
	struct individual_message * imsg = r->individual_messages;
	while (imsg && (imsg)->viewer!=viewer) imsg = imsg->next;
	if (imsg) return imsg->msgs;
	return NULL;
}

struct message * 
r_addmessage(struct region * r, const struct faction * viewer, struct message * msg)
{
	struct individual_message * imsg;
	assert(r);
	imsg = r->individual_messages;
	while (imsg && imsg->viewer!=viewer) imsg = imsg->next;
	if (imsg==NULL) {
		imsg = malloc(sizeof(struct individual_message));
		imsg->next = r->individual_messages;
		imsg->msgs = NULL;
		r->individual_messages = imsg;
		imsg->viewer = viewer;
	}
	return add_message(&imsg->msgs, msg);
}

struct unit * 
region_owner(const struct region * r)
{
#ifdef REGIONOWNERS
  struct unit * owner = NULL;
  int maxsize = 0;
  building * b = r->buildings;
  for (;b!=NULL;b=b->next) {
    if (b->size>maxsize) {
      struct unit * u = buildingowner(r, b);
      if (u) {
        owner = u;
        maxsize=b->size;
      }
    }
  }
  return owner;
#else
  return NULL;
#endif
}
