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
#include "border.h"

#include "unit.h"
#include "region.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
		border * b;
		for (b=borders[key];b;b=b->next) {
			if (b->id==id) return b;
		}
	}
	return NULL;
}

void *
resolve_borderid(void * id) {
   return (void*)find_border((unsigned int)id);
}

static border **
get_borders_i(const region * r1, const region * r2)
{
  border ** bp;
  int key = region_hashkey(r1);
  int k2 = region_hashkey(r2);

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

void
write_borders(FILE * f)
{
	int i;
	for (i=0;i!=BMAXHASH;++i) {
		border * b;
		for (b=borders[i];b;b=b->nexthash) {
			if (b->type->valid && !b->type->valid(b)) continue;
			fprintf(f, "%s %d %d %d %d %d ", b->type->__name, b->id, b->from->x, b->from->y, b->to->x, b->to->y);
			if (b->type->write) b->type->write(b, f);
			putc('\n', f);
#if	RELEASE_VERSION>BORDER_VERSION
			a_write(f, b->attribs);
			putc('\n', f);
#endif
		}
	}
	fputs("end", f);
}

void
read_borders(FILE * f)
{
	for (;;) {
		int fx, fy, tx, ty;
		unsigned int bid = 0;
		char zText[32];
		border * b;
		region * from, * to;
		border_type * type;

		fscanf(f, "%s", zText);
		if (!strcmp(zText, "end")) break;
		fscanf(f, "%u %d %d %d %d", &bid, &fx, &fy, &tx, &ty);
		type = find_bordertype(zText);
		assert(type || !"border type not registered");
		from = findregion(fx, fy);
		if (from==NULL) {
			log_error(("border for unknown region %d,%d\n", fx, fy));
			from = new_region(fx, fy);
		}
		to = findregion(tx, ty);
		if (to==NULL)  {
			log_error(("border for unknown region %d,%d\n", tx, ty));
			to = new_region(tx, ty);
		}
		if (to==from) {
			direction_t dir = (direction_t) (rand() % MAXDIRECTIONS);
			region * r = rconnect(from, dir);
			log_error(("[read_borders] invalid %s in %s\n", type->__name, regionname(from, NULL)));
			if (r!=NULL) to = r;
		}
		b = new_border(type, from, to);
		nextborder--; /* new_border erhˆht den Wert */
		b->id = bid;
		assert(bid<=nextborder);
		if (type->read) type->read(b, f);
		a_read(f, &b->attribs);
	}
}

border *
new_border(border_type * type, region * from, region * to)
{
  border ** bp = get_borders_i(from, to);
  border * b = calloc(1, sizeof(struct border));

  b->type = type;
  b->nexthash = *bp;
  b->next = *bp;
  b->from = from;
  b->to = to;
  b->id = ++nextborder;
  *bp = b;

  if (type->init) type->init(b);
  return b;
}

void
erase_border(border * b)
{
  border ** bp = get_borders_i(b->from, b->to);
  border ** np = NULL;
  attrib ** ap = &b->attribs;

  while (*ap) a_remove(ap, *ap);

  while (*bp && *bp != b) {
    border * bprev = *bp;
    if (bprev->next==b) np=&bprev->next;
    bp = &bprev->next;
  }
  assert(*bp!=NULL || !"error: border is not registered");
  if (np!=NULL) *np = b->next;
  *bp = b->nexthash;
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
	assert(sizeof(int)==sizeof(b->data));
	fscanf(f, "%x ", (unsigned int*)&b->data);
}

void
b_write(const border * b, FILE *f)
{
	assert(sizeof(int)==sizeof(b->data));
	fprintf(f, "%x ", (int)b->data);
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
	DEFAULT_WRITE,
	DEFAULT_READ
};

void
age_borders(void)
{
	int i;

	for (i=0;i!=BMAXHASH;++i) {
		border ** bp = &borders[i];
		while (*bp) {
			border * b = *bp;
			attrib ** ap = &b->attribs;
			while (*ap) {
				attrib * a = *ap;
				if (a->type->age && a->type->age(a)==0) {
					if (a->type == &at_countdown) {
						erase_border(b);
						b = NULL;
						/* bp bleibt, wie es ist, da der
						 * n‰chste border nun hier drin steht. */
						break;
					}
					a_remove(ap, a);
				}
				else ap=&a->next;
			}
      if (b!=NULL) {
        /* bei lˆschung zeigt bp bereits auf den n‰chsten */
        bp=&b->nexthash;
      }
		}
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
	unused(f);
	unused(r);
	unused(b);
	if (gflags & GF_ARTICLE) return "eine Wand";
	return "Wand";
}

border_type bt_wall = {
	"wall",
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
	"noway",
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
	if (gflags & GF_ARTICLE) return "eine Nebelwand";
	return "Nebelwand";
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
	"fogwall",
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
	/* TODO: f->locale bestimmt die Sprache */
	int fno = (int)b->data;
	unused(b);
	unused(r);
	if (gflags & GF_ARTICLE) return (f && fno==f->no)?"eine Illusionswand":"eine Wand";
	return (f && fno==f->no)?"Illusionswand":"Wand";
}

border_type bt_illusionwall = {
	"illusionwall",
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
	if((int)b->data > 0) return true;
	return false;
}

static const char *
b_namequestportal(const border * b, const region * r, const struct faction * f, int gflags)
{
	/* TODO: f->locale bestimmt die Sprache */
	int lock = (int)b->data;
	unused(b);
	unused(r);

	if (gflags & GF_ARTICLE) {
		if(lock > 0) {
			return "ein gewaltiges verschlossenes Tor";
		} else {
			return "ein gewaltiges offenes Tor";
		}
	} else {
		if(lock > 0) {
			return "gewaltiges verschlossenes Tor";
		} else {
			return "gewaltiges offenes Tor";
		}
	}
}

border_type bt_questportal = {
	"questportal",
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
	int rval = (int)b->data;
	int local = (r==b->to)?(rval & 0xFFFF):((rval>>16) & 0xFFFF);
	static char buffer[64];

	unused(f);
	if (gflags & GF_ARTICLE) {
		if (!(gflags & GF_DETAILED)) strcpy(buffer, "eine Straﬂe");
		else if (terrain[rterrain(r)].roadreq<=local) {
			int remote = (r!=b->to)?(rval & 0xFFFF):((rval>>16) & 0xFFFF);
			if (terrain[rterrain(r2)].roadreq<=remote) {
				strcpy(buffer, "eine Straﬂe");
			} else {
				strcpy(buffer, "eine unvollst‰ndige Straﬂe");
			}
		} else {
			int percent = max(1, 100*local/terrain[rterrain(r)].roadreq);
			if (local) {
				sprintf(buffer, "eine zu %d%% vollendete Straﬂe", percent);
			} else {
				strcpy(buffer, "ein Straﬂenanschluﬂ");
			}
		}
	}
	else if (gflags & GF_PLURAL) return "Straﬂen";
	else return "Straﬂe";
	return buffer;
}

static void
b_readroad(border * b, FILE *f)
{
	int x, y;
	fscanf(f, "%d %d", &x, &y);
	b->data=(void*)((x<<16) | y);
}

static void
b_writeroad(const border * b, FILE *f)
{
	int x, y;
	x = (int)b->data;
	y = x & 0xFFFF;
	x = (x >> 16) & 0xFFFF;
	fprintf(f, "%d %d", x, y);
}

static boolean
b_validroad(const border * b)
{
	int x = (int)b->data;
	if (x==0) return false;
	return true;
}

static boolean
b_rvisibleroad(const border * b, const region * r)
{
	int x = (int)b->data;
	x = (r==b->to)?(x & 0xFFFF):((x>>16) & 0xFFFF);
	if (x==0) return false;
	return (boolean)(b->to==r || b->from==r);
}

border_type bt_road = {
	"road",
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

