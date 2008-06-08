/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <kernel/eressea.h>
#include "autoseed.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/plane.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <attributes/key.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/goodies.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/rng.h>
#include <util/sql.h>
#include <util/unicode.h>

#include <libxml/encoding.h>

/* libc includes */
#include <limits.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

const terrain_type *
random_terrain(boolean distribution)
{
  static int nterrains;
  static int ndistribution;
  const terrain_type * terrain;
  int n;
  if (nterrains==0) {
    for (terrain=terrains();terrain;terrain=terrain->next) {
      ndistribution += terrain->distribution;
      ++nterrains;
    }
  }

  n = rng_int() % (distribution?ndistribution:nterrains);
  for (terrain=terrains();terrain;terrain=terrain->next) {
    n -= distribution?terrain->distribution:1;
    if (n<0) break;
  }
  return terrain;
}

int
seed_adamantium(region * r, int base)
{
  const resource_type * rtype = rt_find("adamantium");
  rawmaterial * rm;
  for (rm = r->resources;rm;rm=rm->next) {
    if (rm->type->rtype==rtype) break;
  }
  if (!rm) {
    add_resource(r, 1, base, 150, rtype);
  }
  return 0;
}


static int
count_demand(const region *r)
{
  struct demand *dmd;
  int c = 0;
  if (r->land) {
    for (dmd=r->land->demands;dmd;dmd=dmd->next) c++;
  }
  return c;
}

static int
recurse_regions(region *r, region_list **rlist, boolean(*fun)(const region *r))
{
  if (!fun(r)) return 0;
  else {
    int len = 0;
    direction_t d;
    region_list * rl = calloc(sizeof(region_list), 1);
    rl->next = *rlist;
    rl->data = r;
    (*rlist) = rl;
    fset(r, RF_MARK);
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * nr = rconnect(r, d);
      if (nr && !fval(nr, RF_MARK)) len += recurse_regions(nr, rlist, fun);
    }
    return len+1;
  }
}

static boolean
f_nolux(const region * r)
{
  if (r->land && count_demand(r) != get_maxluxuries()) return true;
  return false;
}

int
fix_demand(region *rd)
{
  region_list *rl, *rlist = NULL;
  static const struct luxury_type **mlux = 0, ** ltypes;
  const luxury_type *sale = NULL;
  int maxlux = 0;
  int maxluxuries = get_maxluxuries();
  
  if (maxluxuries==0) return 0;
  recurse_regions(rd, &rlist, f_nolux);
  if (mlux==0) {
    int i = 0;
    mlux = (const luxury_type **)gc_add(calloc(maxluxuries, sizeof(const luxury_type *)));
    ltypes = (const luxury_type **)gc_add(calloc(maxluxuries, sizeof(const luxury_type *)));
    for (sale=luxurytypes;sale;sale=sale->next) {
      ltypes[i++] = sale;
    }
  }
  else {
    int i;
    for (i=0;i!=maxluxuries;++i) mlux[i] = 0;
  }
  for (rl=rlist;rl;rl=rl->next) {
    region * r = rl->data;
    direction_t d;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * nr = rconnect(r, d);
      if (nr && nr->land && nr->land->demands) {
        struct demand * dmd;
        for (dmd = nr->land->demands;dmd;dmd=dmd->next) {
          if (dmd->value == 0) {
            int i;
            for (i=0;i!=maxluxuries;++i) {
              if (mlux[i]==NULL) {
                maxlux = i;
                mlux[i] = dmd->type;
                break;
              } else if (mlux[i]==dmd->type) {
                break;
              }
            }
            break;
          }
        }
      }
    }
    freset(r, RF_MARK); /* undo recursive marker */
  }
  if (maxlux<2) {
    int i;
    for (i=maxlux;i!=2;++i) {
      int j;
      do {
        int k = rng_int() % maxluxuries;
        mlux[i] = ltypes[k];
        for (j=0;j!=i;++j) {
          if (mlux[j]==mlux[i]) break;
        }
      } while (j!=i);
    }
    maxlux = 2;
  }
  for (rl=rlist;rl;rl=rl->next) {
    region * r = rl->data;
    if (!fval(r, RF_CHAOTIC)) {
      log_error(("fixing demand in %s\n", regionname(r, NULL)));
    }
    sale = mlux[rng_int() % maxlux];
    if (sale) setluxuries(r, sale);
  }
  while (rlist) {
    rl = rlist->next;
    free(rlist);
    rlist = rl;
  }
  return 0;
}

/* nach 150 Runden ist Neustart erlaubt */
#define MINAGE_MULTI 150
newfaction *
read_newfactions(const char * filename)
{
  newfaction * newfactions = NULL;
  FILE * F = fopen(filename, "r");
  char buf[1024];

  if (F==NULL) return NULL;
  for (;;) {
    faction * f;
    char race[20], email[64], lang[8], password[16];
    newfaction *nf, **nfi;
    int bonus = 0, subscription = 0;
    int alliance = 0;

    if (fgets(buf, sizeof(buf), F)==NULL) break;

    email[0] = '\0';
    password[0] = '\0';

    if (sscanf(buf, "%54s %20s %8s %d %d %16s %d", email, race, lang, &bonus, &subscription, password, &alliance)<6) break;
    if (email[0]=='\0') break;
    if (password[0]=='\0') {
      strcpy(password, itoa36(rng_int()));
      strcat(password, itoa36(rng_int()));
    }
    for (f=factions;f;f=f->next) {
      if (strcmp(f->email, email)==0 && f->subscription && f->age<MINAGE_MULTI) break;
    }
    if (f && f->units) continue; /* skip the ones we've already got */
    for (nf=newfactions;nf;nf=nf->next) {
      if (strcmp(nf->email, email)==0) break;
    }
    if (nf) continue;
    nf = calloc(sizeof(newfaction), 1);
    if (set_email(&nf->email, email)!=0) {
      log_error(("Invalid email address for subscription %s: %s\n", 
                 itoa36(subscription), email));
      continue;
    }
    nf->password = strdup(password);
    nf->race = rc_find(race);
    nf->subscription = subscription;
    if (alliances!=NULL) {
      struct alliance * al = findalliance(alliance);
      if (al==NULL) {
        char zText[64];
        sprintf(zText, "Allianz %d", alliance);
        al = makealliance(alliance, zText);
      }
      nf->allies = al;
    } else {
      nf->allies = NULL;
    }
    if (nf->race==NULL) {
      /* if the script didn't supply the race as a token, then it gives us a
       * race in the default locale (which means that itis a UTF8 string) */
      nf->race = findrace(race, default_locale);
      if (nf->race==NULL) {
        char buffer[32];
        size_t outbytes = sizeof(buffer) - 1;
        size_t inbytes = strlen(race);
        unicode_latin1_to_utf8(buffer, &outbytes, race, &inbytes);
        buffer[outbytes] = 0;
        nf->race = findrace(buffer, default_locale);
        if (nf->race==NULL) {
          log_error(("new faction has unknown race '%s'.\n", race));
          free(nf);
          continue;
        }
      }
    }
    nf->lang = find_locale(lang);
    nf->bonus = bonus;
    assert(nf->race && nf->email && nf->lang);
    nfi = &newfactions;
    while (*nfi) {
      if ((*nfi)->race==nf->race) break;
      nfi=&(*nfi)->next;
    }
    nf->next = *nfi;
    *nfi = nf;
  }
  fclose(F);
  return newfactions;
}

extern int numnewbies;

static terrain_t
preferred_terrain(const struct race * rc)
{
	if (rc==rc_find("dwarf")) return T_MOUNTAIN;
	if (rc==rc_find("insect")) return T_DESERT;
	if (rc==rc_find("halfling")) return T_SWAMP;
	if (rc==rc_find("troll")) return T_MOUNTAIN;
	return T_PLAIN;
}

#define REGIONS_PER_FACTION 2
#define PLAYERS_PER_ISLAND 20
#define MAXISLANDSIZE 50
#define MINFACTIONS 1
#define VOLCANO_CHANCE 100

static boolean
virgin_region(const region * r)
{
  direction_t d;
  if (r==NULL) return true;
  if (fval(r->terrain, FORBIDDEN_REGION)) return false;
  if (r->units) return false;
  for (d=0;d!=MAXDIRECTIONS;++d) {
    const region * rn = rconnect(r, d);
    if (rn) {
      if (rn->age>r->age+1) return false;
      if (rn->units) return false;
      if (fval(rn->terrain, FORBIDDEN_REGION)) {
        /* because it kinda sucks to have islands that are adjacent to a firewall */
        return false;
      }
    }
  }
  return true;
}

void
get_island(region * root, region_list ** rlist)
{
  region_list ** rnext = rlist;
  while (*rnext) rnext=&(*rnext)->next;

  fset(root, RF_MARK);
  add_regionlist(rnext, root);

  while (*rnext) {
    direction_t dir;

    region * rcurrent = (*rnext)->data;
    rnext = &(*rnext)->next;

    for (dir=0;dir!=MAXDIRECTIONS;++dir) {
      region * r = rconnect(rcurrent, dir);
      if (r!=NULL && r->land && !fval(r, RF_MARK)) {
        fset(r, RF_MARK);
        add_regionlist(rnext, r);
      }
    }
  }
  rnext=rlist;
  while (*rnext) {
    region_list * rptr = *rnext;
    freset(rptr->data, RF_MARK);
    rnext = &rptr->next;
  }
}

static void
get_island_info(region * root, int * size_p, int * inhabited_p, int * maxage_p)
{
  int size = 0, maxage = 0, inhabited = 0;
  region_list * rlist = NULL;
  region_list * island = NULL;
  add_regionlist(&rlist, root);
  island = rlist;
  fset(root, RF_MARK);
  while (rlist) {
    direction_t d;
    region * r = rlist->data;
    if (r->units) {
      unit * u;
      for (u=r->units; u; u=u->next) {
        if (!is_monsters(u->faction) && u->faction->age > maxage) {
          maxage = u->faction->age;
        }
      }
      ++inhabited;
    }
    ++size;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * rn = rconnect(r, d);
      if (rn && !fval(rn, RF_MARK) && rn->land) {
        region_list * rnew = malloc(sizeof(region_list));
        rnew->data = rn;
        rnew->next = rlist->next;
        rlist->next = rnew;
        fset(rn, RF_MARK);
      }
    }
    rlist = rlist->next;
  }
  for (rlist=island;rlist;rlist=rlist->next) {
    freset(rlist->data, RF_MARK);
  }
  free_regionlist(island);
  if (size_p) *size_p = size;
  if (inhabited_p) *inhabited_p = inhabited;
  if (maxage_p) *maxage_p = maxage;
}

void
free_newfaction(newfaction * nf)
{
  free(nf->email);
  free(nf->password);
  free(nf);
}
/** create new island with up to nsize players
 * returns the number of players placed on the new island.
 */
static void
frame_regions(int age, terrain_t terrain)
{
  region * r = regions;
  for (r=regions;r;r=r->next) {
    direction_t d;
    if (r->age<age) continue;
    if (r->planep) continue;
    if (rterrain(r)==terrain) continue;

    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * rn = rconnect(r, d);
      if (rn==NULL) {
        rn = new_region(r->x+delta_x[d], r->y+delta_y[d], 0);
        terraform(rn, terrain);
        rn->age=r->age;
      }
    }
  }
}
static void
prepare_starting_region(region * r)
{
  int n, t;
  double p;

  assert(r->land);

  /* population between 30% and 60% of max */
  p = rng_double();
  n = (int)(r->terrain->size * (0.3 + p*0.3));
  rsetpeasants(r, n);

  /* trees: don't squash the peasants, and at least 5% should be forrest */
  t = (rtrees(r, 2) + rtrees(r, 1)/2) * TREESIZE;
  if (t < r->terrain->size/20 || t+n > r->terrain->size) {
    double p2 = 0.05 + rng_double()*(1.0-p-0.05);
    int maxtrees = (int)(r->terrain->size/1.25/TREESIZE); /* 1.25 = each young tree will take 1/2 the space of old trees */
    int trees = (int)(p2 * maxtrees);

    rsettrees(r, 2, trees);
    rsettrees(r, 1, trees/2);
    rsettrees(r, 0, trees/4);
  }

  /* horses: between 1% and 2% */
  p = rng_double();
  rsethorses(r, (int)(r->terrain->size * (0.01 + p*0.01)));

  fix_demand(r);
}

int
autoseed(newfaction ** players, int nsize, int max_agediff)
{
  short x = 0, y = 0;
  region * r = NULL;
  region_list * rlist = NULL;
  int rsize = 0, tsize = 0;
  int isize = REGIONS_PER_FACTION; /* target size for the island */
  int psize = 0; /* players on this island */
  const terrain_type * volcano_terrain = get_terrain("volcano");

  frame_regions(16, T_FIREWALL);

  if (listlen(*players)<MINFACTIONS) return 0;

  if (max_agediff>0) {
    region * rmin = NULL;
    /* find a spot that's adjacent to the previous island, but virgin.
     * like the last land virgin ocean region adjacent to land.
     */
    for (r=regions;r;r=r->next) {
      struct plane * p = r->planep;
      if (r->age<=max_agediff && rterrain(r)==T_OCEAN && p==NULL && virgin_region(r)) {
        direction_t d;
        for (d=0;d!=MAXDIRECTIONS;++d) {
          region * rn = rconnect(r, d);
          if (rn && rn->land && rn->age<=max_agediff && virgin_region(rn)) {
            /* only expand islands that aren't single-islands and not too big already */
            int size, inhabitants, maxage;
            get_island_info(rn, &size, &inhabitants, &maxage);
            if (maxage<=max_agediff && size>=2 && size<MAXISLANDSIZE) {
              rmin = rn;
              break;
            }
          }
        }
     } 
    }
    if (rmin!=NULL) {
      region_list * rlist = NULL, * rptr;
      faction * f;
      get_island(rmin, &rlist);
      for (rptr=rlist;rptr;rptr=rptr->next) {
        region * r = rlist->data;
        unit * u;
        for (u=r->units;u;u=u->next) {
          f = u->faction;
          if (!fval(f, FFL_MARK)) {
            ++psize;
            fset(f, FFL_MARK);
          }
        }
      }
      free_regionlist(rlist);
      if (psize>0) for (f=factions;f;f=f->next) freset(f, FFL_MARK);
      if (psize<PLAYERS_PER_ISLAND) {
        r = rmin;
      }
    }
  }
  if (r==NULL) {
    region * rmin = NULL;
    direction_t dmin = MAXDIRECTIONS;
    /* find an empty spot.
     * rmin = the youngest ocean region that has a missing neighbour 
     * dmin = direction in which it's empty
     */
    for (r=regions;r;r=r->next) {
      struct plane * p = r->planep;
      if (rterrain(r)==T_OCEAN && p==0 && (rmin==NULL || r->age<=max_agediff)) {
        direction_t d;
        for (d=0;d!=MAXDIRECTIONS;++d) {
          region * rn  = rconnect(r, d);
          if (rn==NULL) break;
        }
        if (d!=MAXDIRECTIONS) {
          rmin=r;
          dmin=d;
        }
      }
    }

    /* create a new region where we found the empty spot, and make it the first 
    * in our island. island regions are kept in rlist, so only new regions can 
    * get populated, and old regions are not overwritten */
    if (rmin!=NULL) {
      assert(virgin_region(rconnect(rmin, dmin)));
      x = rmin->x + delta_x[dmin];
      y = rmin->y + delta_y[dmin];
      r = new_region(x, y, 0);
      terraform(r, T_OCEAN); /* we change the terrain later */
    }
  }
  if (r!=NULL) {
    add_regionlist(&rlist, r);
    fset(r, RF_MARK);
    rsize = 1;
  }

  while (rsize && (nsize || isize>=REGIONS_PER_FACTION)) {
    int i = rng_int() % rsize;
    region_list ** rnext = &rlist;
    region_list * rfind;
    direction_t d;
    while (i--) rnext=&(*rnext)->next;
    rfind = *rnext;
    r = rfind->data;
    freset(r, RF_MARK);
    *rnext = rfind->next;
    free(rfind);
    --rsize;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * rn = rconnect(r, d);
      if (rn && fval(rn, RF_MARK)) continue;
      if (rn==NULL) {
        rn = new_region(r->x + delta_x[d], r->y + delta_y[d], 0);
        terraform(rn, T_OCEAN);
      }
      if (virgin_region(rn)) {
        add_regionlist(&rlist, rn);
        fset(rn, RF_MARK);
        ++rsize;
      }
    }
    if (volcano_terrain!=NULL && (rng_int() % VOLCANO_CHANCE == 0)) {
      terraform_region(r, volcano_terrain);
    } else if (nsize && (rng_int() % isize == 0 || rsize==0)) {
      newfaction ** nfp, * nextf = *players;
      faction * f;
      unit * u;

      isize += REGIONS_PER_FACTION;
      terraform(r, preferred_terrain(nextf->race));
      prepare_starting_region(r);
      ++tsize;
      assert(r->land && r->units==0);
      u = addplayer(r, addfaction(nextf->email, nextf->password, nextf->race,
                                  nextf->lang, nextf->subscription));
      f = u->faction;
      fset(f, FFL_ISNEW);
			f->alliance = nextf->allies;
			log_printf("New faction (%s), %s at %s\n", itoa36(f->no),
								 f->email, regionname(r, NULL));
      if (f->subscription) {
        sql_print(("UPDATE subscriptions SET status='ACTIVE', faction='%s', firstturn=%d, lastturn=%d, password='%s' WHERE id=%u;\n",
          factionid(f), f->lastorders, f->lastorders, f->override, f->subscription));
      }

      /* remove duplicate email addresses */
      nfp = &nextf->next;
      while (*nfp) {
        newfaction * nf = *nfp;
        if (strcmp(nextf->email, nf->email)==0) {
          *nfp = nf->next;
          free_newfaction(nf);
        }
        else nfp = &nf->next;
      }
      *players = nextf->next;
      free_newfaction(nextf);

      ++psize;
      --nsize;
      --isize;
      if (psize>=PLAYERS_PER_ISLAND) break;
    } else {
      terraform_region(r, random_terrain(true));
      --isize;
    }
  }

  if (nsize!=0) {
    log_error(("Could not place all factions on the same island as requested\n"));
  }
  
  
  if (rlist) {
#define MINOCEANDIST 3
#define MAXOCEANDIST 6
#define MAXFILLDIST 10
#define SPECIALCHANCE 80
    region_list ** rbegin = &rlist;
    int special = 1;
    int oceandist = MINOCEANDIST + (rng_int() % (MAXOCEANDIST-MINOCEANDIST));
    while (oceandist--) {
      region_list ** rend = rbegin;
      while (*rend) rend=&(*rend)->next;
      while (rbegin!=rend) {
        direction_t d;
        region * r = (*rbegin)->data;
        rbegin=&(*rbegin)->next;
        for (d=0;d!=MAXDIRECTIONS;++d) {
          region * rn = rconnect(r, d);
          if (rn==NULL) {
            const struct terrain_type * terrain = newterrain(T_OCEAN);
            rn = new_region(r->x + delta_x[d], r->y + delta_y[d], 0);
            if (rng_int() % SPECIALCHANCE < special) {
              terrain = random_terrain(true);
              special = SPECIALCHANCE / 3; /* 33% chance auf noch eines */
            } else {
              special = 1;
            }
            terraform_region(rn, terrain);
            /* the new region has an extra 20% chance to have mallorn */
            if (rng_int() % 100 < 20) fset(r, RF_MALLORN);
            add_regionlist(rend, rn);
          }
        }
      }
      
    }
    while (*rbegin) {
      region * r = (*rbegin)->data;
      direction_t d;
      rbegin=&(*rbegin)->next;
      for (d=0;d!=MAXDIRECTIONS;++d) if (rconnect(r, d)==NULL) {
        short i;
        for (i=1;i!=MAXFILLDIST;++i) {
          if (findregion(r->x + i*delta_x[d], r->y + i*delta_y[d]))
            break;
          
        }
        if (i!=MAXFILLDIST) {
          while (--i) {
            region * rn = new_region(r->x + i*delta_x[d], r->y + i*delta_y[d], 0);
            terraform(rn, T_OCEAN);
          }
        }
      }
    }
    while (rlist) {
      region_list * self = rlist;
      rlist = rlist->next;
      freset(self->data, RF_MARK);
      free(self);
    }
  }
  return tsize;
}
