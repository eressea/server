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
#include "faction.h"
#include "plane.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>

#define TE_CENTER_X 1000
#define TE_CENTER_Y 1000
#define TP_RADIUS 4

static int
real2tp(int rk) {
  /* in C:
  * -4 / 5 = 0;
  * +4 / 5 = 0;
  * !!!!!!!!!!;
  */
  return (rk + (TP_RADIUS*5000)) / TP_RADIUS - 5000;
}

static region *
tpregion(const region *r) {
  return findregion(TE_CENTER_X+real2tp(r->x), TE_CENTER_Y+real2tp(r->y));
}

region_list *
astralregions(const region * r, boolean (*valid)(const region *))
{
  region_list * rlist = NULL;
  int x, y;

  assert(rplane(r) == NULL);
  if (r==NULL) return NULL;
  r = r_astral_to_standard(r);

  for (x=r->x-TP_RADIUS;x<=r->x+TP_RADIUS;++x) {
    for (y=r->y-TP_RADIUS;y<=r->y+TP_RADIUS;++y) {
      region * rn;
      int dist = koor_distance(r->x, r->y, x, y);

      if (dist > TP_RADIUS) continue;
      if (dist==4) {
        /* these three regions are in dispute with the NW, W & SW areas */
        if (x==-2 && y==0) continue;
        if (x==-2 && y==2) continue;
        if (x==0 && y==-2) continue;
      }
      rn = findregion(x, y);
      if (rn!=NULL && (valid==NULL || valid(rn))) add_regionlist(&rlist, rn);
    }
  }
  return rlist;
}

region *
r_standard_to_astral(const region *r)
{
  region *r2;
  r2 = tpregion(r);

  if (rplane(r2) != get_astralplane() || rterrain(r2) != T_ASTRAL) return NULL;

  return r2;
}

region *
r_astral_to_standard(const region *r)
{
  int x, y;
  region *r2;

  assert(rplane(r) == get_astralplane());
  x = (r->x-TE_CENTER_X)*TP_RADIUS;
  y = (r->y-TE_CENTER_Y)*TP_RADIUS;

  r2 = findregion(x,y);
  if (r2!=NULL || getplaneid(r2) != 0) return NULL;

  return r2;
}

region_list *
all_in_range(const region *r, int n, boolean (*valid)(const region *))
{
  int x,y;
  region_list *rlist = NULL;

  if (r == NULL) return NULL;

  for (x = r->x-n; x <= r->x+n; x++) {
    for (y = r->y-n; y <= r->y+n; y++) {
      if (koor_distance(r->x, r->y, x, y) <= n) {
        region * r2 = findregion(x,y);
        if (r2!=NULL && (valid==NULL || valid(r2))) add_regionlist(&rlist, r2);
      }
    }
  }

  return rlist;
}

void
random_in_teleport_plane(void)
{
  region *r;
  faction *f0 = findfaction(MONSTER_FACTION);
  int next = rand() % 100;

  if (f0==NULL) return;

  for (r=regions; r; r=r->next) {
    if (rplane(r) != get_astralplane() || rterrain(r) != T_ASTRAL) continue;

    /* Neues Monster ? */
    if (next-- == 0) {
      unit *u = createunit(r, f0, 1+rand()%10+rand()%10, new_race[RC_HIRNTOETER]);

      set_string(&u->name, "Hirntöter");
      set_string(&u->display, "Wabernde grüne Schwaden treiben durch den Nebel und verdichten sich zu einer unheimlichen Kreatur, die nur aus einem langen Ruderschwanz und einem riesigen runden Maul zu bestehen scheint.");
      set_level(u, SK_STEALTH, 1);
      set_level(u, SK_OBSERVATION, 1);
      next = rand() % 100;
    }
  }
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
  int i;
  plane * aplane = get_astralplane();

  /* Regionsbereich aufbauen. */
  /* wichtig: das muß auch für neue regionen gemacht werden.
  * Evtl. bringt man es besser in new_region() unter, und
  * übergibt an new_region die plane mit, in der die
  * Region gemacht wird.
  */

  for (r=regions;r;r=r->next) if(r->planep == NULL) {
    region *ra = tpregion(r);
    if (ra==NULL) {
      ra = new_region(TE_CENTER_X+real2tp(r->x), TE_CENTER_Y+real2tp(r->y));
      rsetterrain(ra, T_ASTRAL);
    }
    ra->planep  = aplane;
    if (terrain[rterrain(r)].flags & FORBIDDEN_LAND) rsetterrain(ra, T_ASTRALB);
  }

  for(i=0;i<4;i++) {
    random_in_teleport_plane();
  }
}

boolean 
inhabitable(const region * r)
{
  return landregion(rterrain(r));
}
