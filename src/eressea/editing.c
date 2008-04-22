/* vi: set ts=2:
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2006   |  
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *  
 */

#include <config.h>
#include <kernel/eressea.h>
#include "editing.h"

#include <kernel/region.h>
#include <modules/autoseed.h>
#include <util/rng.h>
#include <util/lists.h>

#define ISLANDSIZE 20
#define TURNS_PER_ISLAND 4
void
seed_players(const char * filename, boolean new_island)
{
  newfaction * players = read_newfactions(filename);
  if (players!=NULL) {
    rng_init(players->subscription);
    while (players) {
      int n = listlen(players);
      int k = (n+ISLANDSIZE-1)/ISLANDSIZE;
      k = n / k;
      n = autoseed(&players, k, new_island || (turn % TURNS_PER_ISLAND)==0);
      if (n==0) {
        break;
      }
    }
  }
}

void
make_block(short x, short y, short radius, const struct terrain_type * terrain)
{
  short cx, cy;
  region *r;

  if (terrain==NULL) return;

  for (cx = x - radius; cx != x+radius; ++cx) {
    for (cy = y - radius; cy != y+radius; ++cy) {
      if (koor_distance(cx, cy, x, y) < radius) {
        if (!findregion(cx, cy)) {
          r = new_region(cx, cy, 0);
          terraform_region(r, terrain);
        }
      }
    }
  }
}

void
make_island(short x, short y, int size)
{
  /* region_list * island; */
}
