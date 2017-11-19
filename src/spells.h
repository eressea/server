/* 
 *
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

#ifndef H_SPL_SPELLS
#define H_SPL_SPELLS

#include "magic.h"


#ifdef __cplusplus
extern "C" {
#endif

    struct castorder;
    struct curse_type;
    struct region;
    struct unit;
    struct message;

    extern const struct curse_type ct_magicresistance;

    void register_magicresistance(void);
    void register_spells(void);

    int sp_baddreams(struct castorder * co);
    int sp_gooddreams(struct castorder * co);
    int sp_viewreality(struct castorder * co);

#define ACTION_RESET      0x01  /* reset the one-time-flag FFL_SELECT (on first pass) */
#define ACTION_CANSEE     0x02  /* to people who can see the actor */
#define ACTION_CANNOTSEE  0x04  /* to people who can not see the actor */
    int report_action(struct region *r, struct unit *actor, struct message *msg, int flags);

#ifdef __cplusplus
}
#endif
#endif
