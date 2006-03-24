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
#include <eressea.h>
#include "editing.h"

#include <kernel/region.h>

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
          r = new_region(cx, cy);
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
