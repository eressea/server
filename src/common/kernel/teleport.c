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
#include "teleport.h"

/* kernel includes */
#include "unit.h"
#include "region.h"
#include "race.h"
#include "skill.h"
#include "terrain.h"
#include "faction.h"
#include "plane.h"

/* util includes */
#include <util/log.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>

#define TE_CENTER_X 1000
#define TE_CENTER_Y 1000
#define TP_RADIUS 2
#define TP_DISTANCE 4

static short
real2tp(short rk) {
  /* in C:
  * -4 / 5 = 0;
  * +4 / 5 = 0;
  * !!!!!!!!!!;
  */
  return (rk + (TP_DISTANCE*5000)) / TP_DISTANCE - 5000;
}

static region *
tpregion(const region *r) {
  region * rt = findregion(TE_CENTER_X+real2tp(r->x), TE_CENTER_Y+real2tp(r->y));
  if (rplane(rt) != get_astralplane()) return NULL;
  return rt;
}

region_list *
astralregions(const region * r, boolean (*valid)(const region *))
{
  region_list * rlist = NULL;
  short x, y;

  assert(rplane(r) == get_astralplane());
  if (rplane(r) != get_astralplane()) {
    log_error(("astralregions was called with a non-astral region.\n"));
    return NULL;
  }
  r = r_astral_to_standard(r);
  if (r==NULL) return NULL;

  for (x=-TP_RADIUS;x<=+TP_RADIUS;++x) {
    for (y=-TP_RADIUS;y<=+TP_RADIUS;++y) {
      region * rn;
      int dist = koor_distance(0, 0, x, y);

      if (dist > TP_RADIUS) continue;
      rn = findregion(r->x+x, r->y+y);
      if (rn!=NULL && (valid==NULL || valid(rn))) add_regionlist(&rlist, rn);
    }
  }
  return rlist;
}

region *
r_standard_to_astral(const region *r)
{
  region *r2;

  if (rplane(r) != get_normalplane()) return NULL;

  r2 = tpregion(r);
  if (r2==NULL || fval(r2->terrain, FORBIDDEN_REGION)) return NULL;

  return r2;
}

region *
r_astral_to_standard(const region *r)
{
  short x, y;
  region *r2;

  assert(rplane(r) == get_astralplane());
  x = (r->x-TE_CENTER_X)*TP_DISTANCE;
  y = (r->y-TE_CENTER_Y)*TP_DISTANCE;

  r2 = findregion(x,y);
  if (r2==NULL || rplane(r2)!=get_normalplane()) return NULL;

  return r2;
}

region_list *
all_in_range(const region *r, short n, boolean (*valid)(const region *))
{
  short x, y;
  region_list *rlist = NULL;

  if (r == NULL) return NULL;

  for (x = r->x-n; x <= r->x+n; x++) {
    for (y = r->y-n; y <= r->y+n; y++) {
      if (koor_distance(r->x, r->y, x, y) <= n) {
        region * r2 = findregion(x, y);
        if (r2!=NULL && (valid==NULL || valid(r2))) add_regionlist(&rlist, r2);
      }
    }
  }

  return rlist;
}

void
spawn_braineaters(float chance)
{
  region *r;
  faction *f0 = findfaction(MONSTER_FACTION);
  int next = rng_int() % (int)(chance*100);
  
  if (f0==NULL) return;

  for (r = regions; r; r = r->next) {
    if (rplane(r) != get_astralplane() || fval(r->terrain, FORBIDDEN_REGION)) continue;

    /* Neues Monster ? */
    if (next-- == 0) {
      unit *u = createunit(r, f0, 1+rng_int()%10+rng_int()%10, new_race[RC_HIRNTOETER]);

      set_string(&u->name, "Hirntöter");
      set_level(u, SK_STEALTH, 1);
      set_level(u, SK_OBSERVATION, 1);
      next = rng_int() % (int)(chance*100);
    }
  }
}

plane *
get_normalplane(void)
{
  return NULL;
}

plane * 
get_astralplane(void)
{
  static plane * astral_plane = NULL;
  if (astral_plane==NULL) {
    astral_plane = getplanebyid(1);
  }
  if (astral_plane==NULL) {
    astral_plane = create_new_plane(1, "Astralraum",
      TE_CENTER_X-500, TE_CENTER_X+500,
      TE_CENTER_Y-500, TE_CENTER_Y+500, 0);
  }
  return astral_plane;
}

void
create_teleport_plane(void)
{
  region *r;
  plane * aplane = get_astralplane();

  const terrain_type * fog = get_terrain("fog");
  const terrain_type * thickfog = get_terrain("thickfog");

  if (fog==0 || thickfog==0) {
    log_warning(("cannot find terrain-types for astral space.\n"));
    return;
  }
  /* Regionsbereich aufbauen. */
  /* wichtig: das muß auch für neue regionen gemacht werden.
  * Evtl. bringt man es besser in new_region() unter, und
  * übergibt an new_region die plane mit, in der die
  * Region gemacht wird.
  */

  for (r=regions;r;r=r->next) if (r->planep == NULL) {
    region *ra = tpregion(r);
    if (ra==NULL) {
      short x = TE_CENTER_X+real2tp(r->x);
      short y = TE_CENTER_Y+real2tp(r->y);
      plane * pl = findplane(x, y);

      if (pl==aplane) {
        ra = new_region(x, y);
        
        if (fval(r->terrain, FORBIDDEN_REGION)) {
          terraform_region(ra, thickfog);
        } else {
          terraform_region(ra, fog);
        }
        ra->planep = aplane;
      }
    }
  }
}

boolean 
inhabitable(const region * r)
{
  return fval(r->terrain, LAND_REGION);
}
