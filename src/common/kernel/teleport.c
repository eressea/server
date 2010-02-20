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

#include <platform.h>
#include <kernel/eressea.h>
#include "teleport.h"

/* kernel includes */
#include "equipment.h"
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

static int
real2tp(int rk) {
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
  if (!is_astral(rt)) return NULL;
  return rt;
}

region_list *
astralregions(const region * r, boolean (*valid)(const region *))
{
  region_list * rlist = NULL;
  int x, y;

  assert(is_astral(r));
  if (!is_astral(r)) {
    log_error(("astralregions was called with a non-astral region.\n"));
    return NULL;
  }
  r = r_astral_to_standard(r);
  if (r==NULL) return NULL;

  for (x=-TP_RADIUS;x<=+TP_RADIUS;++x) {
    for (y=-TP_RADIUS;y<=+TP_RADIUS;++y) {
      region * rn;
      int dist = koor_distance(0, 0, x, y);
      int nx = r->x+x, ny = r->y+y;

      if (dist > TP_RADIUS) continue;
      pnormalize(&nx, &ny, rplane(r));
      rn = findregion(nx, ny);
      if (rn!=NULL && (valid==NULL || valid(rn))) add_regionlist(&rlist, rn);
    }
  }
  return rlist;
}

region *
r_standard_to_astral(const region *r)
{
  if (rplane(r) != get_normalplane()) return NULL;
  return tpregion(r);
}

region *
r_astral_to_standard(const region *r)
{
  int x, y;
  region *r2;

  assert(is_astral(r));
  x = (r->x-TE_CENTER_X)*TP_DISTANCE;
  y = (r->y-TE_CENTER_Y)*TP_DISTANCE;
  pnormalize(&x, &y, get_normalplane());
  r2 = findregion(x, y);
  if (r2==NULL || rplane(r2)!=get_normalplane()) return NULL;

  return r2;
}

region_list *
all_in_range(const region *r, int n, boolean (*valid)(const region *))
{
  int x, y;
  region_list *rlist = NULL;
  plane * pl = rplane(r);

  if (r == NULL) return NULL;

  for (x = r->x-n; x <= r->x+n; x++) {
    for (y = r->y-n; y <= r->y+n; y++) {
      if (koor_distance(r->x, r->y, x, y) <= n) {
        region * r2;
        int nx = x, ny = y;
        pnormalize(&nx, &ny, pl);
        r2 = findregion(nx, ny);
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
  faction *f0 = get_monsters();
  int next = rng_int() % (int)(chance*100);
  
  if (f0==NULL) return;

  for (r = regions; r; r = r->next) {
    if (!is_astral(r) || fval(r->terrain, FORBIDDEN_REGION)) continue;

    /* Neues Monster ? */
    if (next-- == 0) {
      unit *u = createunit(r, f0, 1+rng_int()%10+rng_int()%10, new_race[RC_HIRNTOETER]);
      equip_unit(u, get_equipment("monster_braineater"));

      next = rng_int() % (int)(chance*100);
    }
  }
}

plane *
get_normalplane(void)
{
  return NULL;
}

boolean
is_astral(const region * r)
{
  plane * pl = get_astralplane();
  return (pl && rplane(r) == pl);
}

plane * 
get_astralplane(void)
{
  static plane * astralspace;
  static int rule_astralplane = -1;
  static int gamecookie = -1;
  if (rule_astralplane<0) {
    rule_astralplane = get_param_int(global.parameters, "modules.astralspace", 1);
  }
  if (!rule_astralplane) {
    return NULL;
  }
  if (gamecookie!=global.cookie) {
    astralspace = getplanebyname("Astralraum");
    gamecookie = global.cookie;
  }

  if (astralspace==NULL) {
    astralspace = create_new_plane(1, "Astralraum",
      TE_CENTER_X-500, TE_CENTER_X+500,
      TE_CENTER_Y-500, TE_CENTER_Y+500, 0);
  }
  return astralspace;
}

void
create_teleport_plane(void)
{
  region *r;
  plane * hplane = get_homeplane();
  plane * aplane = get_astralplane();

  const terrain_type * fog = get_terrain("fog");

  for (r=regions;r;r=r->next) {
    plane * pl = rplane(r);
    if (pl == hplane) {
      region *ra = tpregion(r);

      if (ra==NULL) {
        int x = TE_CENTER_X+real2tp(r->x);
        int y = TE_CENTER_Y+real2tp(r->y);
        pnormalize(&x, &y, aplane);
        
        ra = new_region(x, y, aplane, 0);
        terraform_region(ra, fog);
      }
    }
  }
}

boolean 
inhabitable(const region * r)
{
  return fval(r->terrain, LAND_REGION);
}
