/*
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2007   |
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *
 */

#ifndef H_RACES
#define H_RACES

#ifdef __cplusplus
extern "C" {
#endif
    struct unit;

    void register_races(void);
    void make_undead_unit(struct unit *);
    void equip_newunits(struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
