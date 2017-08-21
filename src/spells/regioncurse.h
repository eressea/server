/* 
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef _RCURSE_H
#define _RCURSE_H
#ifdef __cplusplus
extern "C" {
#endif

    struct curse_type;

    extern const struct curse_type ct_peacezone;
    extern const struct curse_type ct_drought;
    extern const struct curse_type ct_blessedharvest;
    extern const struct curse_type ct_godcursezone;
    extern const struct curse_type ct_gbdream;
    extern const struct curse_type ct_healing;

    void register_regioncurse(void);

#ifdef __cplusplus
}
#endif
#endif                          /* _RCURSE_H */
