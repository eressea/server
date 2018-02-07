/* 
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2005   |  
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 */

#ifndef H_KRNL_TERRAINID_H
#define H_KRNL_TERRAINID_H

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum {
    T_OCEAN = 0,
    T_PLAIN,
    T_SWAMP,
    T_DESERT,
    T_HIGHLAND,
    T_MOUNTAIN,
    T_GLACIER,
    T_FIREWALL,
    T_ASTRAL,
    T_ASTRALB,
    T_VOLCANO,
    T_VOLCANO_SMOKING,
    T_ICEBERG_SLEEP,
    T_ICEBERG,
    MAXTERRAINS,
    NOTERRAIN = - 1
  } terrain_t;

  extern const struct terrain_type *newterrain(terrain_t t);
  extern terrain_t oldterrain(const struct terrain_type *terrain);

#ifdef __cplusplus
}
#endif
#endif
