/* vi: set ts=2:
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2006   |  
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *  
 */

#ifndef GM_EDITING
#define GM_EDITING

struct terrain_type;
extern void make_block(int x, int y, int radius, const struct terrain_type * terrain);
extern void make_island(int x, int y, int size);
extern void seed_players(const char * filename, boolean new_island);

#endif /* GM_EDITING */
