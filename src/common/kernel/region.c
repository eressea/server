/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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
#include "faction.h"
#include "item.h"
#include "plane.h"
#include "region.h"
#include "curse.h"
#include "message.h"

/* util includes */
#include <resolve.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_maxluxuries = 0;

const int delta_x[MAXDIRECTIONS] =
{
	-1, 0, 1, 1, 0, -1
};

const int delta_y[MAXDIRECTIONS] =
{
	1, 1, 0, -1, -1, 0
};

const direction_t back[MAXDIRECTIONS] =
{
	D_SOUTHEAST,
	D_SOUTHWEST,
	D_WEST,
	D_NORTHWEST,
	D_NORTHEAST,
	D_EAST,
};

const char *
regionname(const region * r, const faction * f)
{
	static char buf[65];
	plane *pl = getplane(r);

	if (f == NULL) {
		strncpy(buf, rname(r, NULL), 65);
	} else if (pl && fval(pl, PFL_NOCOORDS)) {
		strncpy(buf, rname(r, f->locale), 65);
	} else {
#ifdef HAVE_SNPRINTF
		snprintf(buf, 65, "%s (%d,%d)", rname(r, f->locale), region_x(r, f), region_y(r, f));
#else
		strncpy(buf, rname(r, f->locale), 50);
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

	if(d->duration > 0) d->duration--;
	else d->duration = 0;

	return d->duration;
}

int
a_readdirection(attrib *a, FILE *f)
{
	spec_direction *d = (spec_direction *)(a->data.v);
	fscanf(f, "%d %d %d", &d->x, &d->y, &d->duration);
	fscanf(f, "%s ", buf);
	d->desc = strdup(cstring(buf));
	fscanf(f, "%s ", buf);
	d->keyword = strdup(cstring(buf));
	return 1;
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
	return 1;
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


#define RMAXHASH 65535
region *regionhash[RMAXHASH];

static region *
rfindhash(int x, int y)
{
	region *old;
	for (old = regionhash[abs(x + 0x100 * y) % RMAXHASH]; old; old = old->nexthash)
		if (old->x == x && old->y == y)
			return old;
	return 0;
}

void
rhash(region * r)
{
	region *old = regionhash[region_hashkey(r) % RMAXHASH];
	regionhash[region_hashkey(r) % RMAXHASH] = r;
	r->nexthash = old;
}

void
runhash(region * r)
{
	region **show;
	for (show = &regionhash[abs(r->x + 0x100 * r->y) % RMAXHASH]; *show; show = &(*show)->nexthash) {
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
rconnect(const region * r, direction_t dir) {
	static int set = 0;
	static region * buffer[MAXDIRECTIONS];
	static const region * last = NULL;
	assert(dir<MAXDIRECTIONS);
	if (r != last) {
		set = 0;
		last = r;
	}
	else
		if (set & (1 << dir)) return buffer[dir];
	buffer[dir] = rfindhash(r->x + delta_x[dir], r->y + delta_y[dir]);
	set |= (1<<dir);
	return buffer[dir];
}

region *
findregion(int x, int y)
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

direction_t
koor_reldirection(int ax, int ay, int bx, int by)
{
	direction_t i;
	for (i=0;i!=MAXDIRECTIONS;++i) {
		if (bx-ax == delta_x[i] &&
			by-ay == delta_y[i]) return i;
	}
	return NODIRECTION;
}

direction_t
reldirection(region * from, region * to)
{
	return koor_reldirection(from->x, from->y, to->x, to->y);
}

void
free_regionlist(regionlist *rl)
{
	while (rl) {
		regionlist * rl2 = rl->next;
		free(rl);
		rl = rl2;
	}
}

regionlist *
add_regionlist(regionlist *rl, region *r)
{
	regionlist *rl2 = calloc(1, sizeof(regionlist));

	rl2->region = r;
	rl2->next   = rl;

	return rl2;
}

#if AT_SALARY
/*****************/
/*   at_salary   */
/*****************/
attrib_type at_salary = {
	"salary",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	NO_WRITE,
	NO_READ,
	ATF_UNIQUE
};
#endif
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

/***************/
/*   at_laen   */
/***************/
attrib_type at_laen = {
	"laen",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	DEFAULT_WRITE,
	DEFAULT_READ,
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
rsetroad(region * r, direction_t d, int val)
{
	int rval;
	border * b;
	region * r2 = rconnect(r, d);

	if (!r2) return;
	b = get_borders(r, r2);
	while (b && b->type!=&bt_road) b = b->next;
	if (!b) b = new_border(&bt_road, r, r2);
	rval = (int)b->data;
	if (b->from==r)
		rval = (rval & 0xFFFF) | (val<<16);
	else
		rval = (rval & 0xFFFF0000) | val;
	b->data = (void*)rval;
}

int
rroad(const region * r, direction_t d)
{
	int rval;
	border * b;
	region * r2 = rconnect(r, d);

	if (!r2) return 0;
	b = get_borders(r, r2);
	while (b && b->type!=&bt_road) b = b->next;
	if (!b) return 0;
	rval = (int)b->data;
	if (b->to==r)
		rval = (rval & 0xFFFF);
	else
		rval = (rval & 0xFFFF0000) >> 16;
	return rval;
}

boolean
r_isforest(const region * r)
{
	if (r->terrain==T_PLAIN && rtrees(r)>=600) return true;
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
rname(const region * r, const locale * lang) {
	if (r->land)
		return r->land->name;
	return locale_string(lang, terrain[rterrain(r)].name);
}

int
rtrees(const region *r)
{
	return ((r)->land?(r)->land->trees:0);
}

int
rsettrees(region *r, int value)
{
	if (!r->land) assert(value==0);
	else {
		assert(value>=0);
		return r->land->trees=value;
	}
	return 0;
}

region *
new_region(int x, int y)
{
	static region *last = 0;
	region *r = rfindhash(x, y);

	if (r) {
		fprintf(stderr, "\ndoppelte regionen entdeckt: %s\n", regionname(r, NULL));
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
	return r;
}

void
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

static char *
makename(void)
{
	int s, v, k, e, p = 0, x = 0;
	int nk, ne, nv, ns;
	static char name[16];
	const char *kons = "bdfghklmnprstvwz",
		*end = "nlrdst",
		*vokal = "aaaaaaaaaßàâeeeeeeeeeéèêiiiiiiiiiíîoooooooooóòôuuuuuuuuuúyy",
		*start = "dgtskpvfr";

	nk = strlen(kons);
	ne = strlen(end);
	nv = strlen(vokal);
	ns = strlen(start);

	for (s = rand() % 3 + 2; s > 0; s--) {
		if (x == 0) {
			k = rand() % nk;
			name[p] = kons[k];
			p++;
		} else {
			k = rand() % ns;
			name[p] = start[k];
			p++;
		}
		v = rand() % nv;
		name[p] = vokal[v];
		p++;
		if (rand() % 3 == 2 || s == 1) {
			e = rand() % ne;
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
	/* defaults: */

	rsetterrain(r, t);
	rsetlaen(r, -1);

	if (!landregion(t)) {
		if (r->land) {
			freeland(r->land);
			r->land = NULL;
		}
		rsettrees(r, 0);
		rsethorses(r, 0);
		rsetiron(r, 0);
		rsetpeasants(r, 0);
		rsetmoney(r, 0);
		rsetlaen(r, -1);
		freset(r, RF_ENCOUNTER);
		freset(r, RF_MALLORN);
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

		if (terrain[r->terrain].herbs) name = terrain[r->terrain].herbs[rand()%3];

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
			rsettrees(r, terrain[T_PLAIN].production_max * (40 + rand() % 40) / 100);
		}
		break;
#ifndef NO_FOREST
	case T_FOREST:

		rsetterrain(r, T_PLAIN);
		rsettrees(r, terrain[T_PLAIN].production_max * (60 + rand() % 30) / 100);
		break;
#endif
	case T_MOUNTAIN:

		rsetiron(r, IRONSTART);
		if (rand() % 100 < 8) rsetlaen(r, 5 + rand() % 5);
		break;

	case T_GLACIER:
		rsetiron(r, GLIRONSTART);
		break;
	}
	{
		if (terrain[t].production_max && !fval(r, RF_CHAOTIC)) {
			int np = MAXPEASANTS_PER_AREA * (rand() % (terrain[t].production_max / 2));
			rsetpeasants(r, max(100, np));
			rsetmoney(r, rpeasants(r) * ((wage(r, NULL, false)+1) + rand() % 5));
		}
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
	if (is_spell_active(r, C_DROUGHT)) p /= 2;

	return p;
}

void
read_region_reference(region ** r, FILE * F)
{
	int x[2];
	fscanf(F, "%d %d", &x[0], &x[1]);
	if (x[0]==INT_MAX) *r = NULL;
	else {
		*r = findregion(x[0], x[1]);
		if (*r==NULL) ur_add(memcpy(malloc(sizeof(x)), x, sizeof(x)), (void**)r, resolve_region);
	}
}

void
write_region_reference(const region * r, FILE * F)
{
	if (r) fprintf(F, "%d %d ", r->x, r->y);
	else fprintf(F, "%d %d ", INT_MAX, INT_MAX);
}

void *
resolve_region(void * id) {
	int * c = (int*)id;
	int x = c[0], y = c[1];
	free(c);
	return findregion(x, y);
}


struct message_list *
r_getmessages(struct region * r, const struct faction * viewer)
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