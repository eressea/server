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

  enum {
    T_OCEAN = 0,
    T_PLAIN = 1,
    T_SWAMP = 2,
    T_DESERT = 3,               /* kann aus T_PLAIN entstehen */
    T_HIGHLAND = 4,
    T_MOUNTAIN = 5,
    T_GLACIER = 6,              /* kann aus T_MOUNTAIN entstehen */
    T_FIREWALL = 7,             /* Unpassierbar */
    /* T_HELL = 8, Hölle */
    /* T_GRASSLAND = 9, */
    T_ASTRAL = 10,
    T_ASTRALB = 11,
    T_VOLCANO = 12,
    T_VOLCANO_SMOKING = 13,
    T_ICEBERG_SLEEP = 14,
    T_ICEBERG = 15,
    /* T_HALL1 = 16, */
    /* T_CORRIDOR1 = 17, */
    /* T_MAGICSTORM = 18, */
    /* T_WALL1 = 19, */
    NOTERRAIN = (terrain_t) - 1
  };

  extern const struct terrain_type *newterrain(terrain_t t);
  extern terrain_t oldterrain(const struct terrain_type *terrain);

#ifdef __cplusplus
}
#endif
#endif
