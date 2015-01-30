/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#ifndef H_GC_COMBATSPELLS
#define H_GC_COMBATSPELLS

#ifdef __cplusplus
extern "C" {
#endif

    struct fighter;

    /* Kampfzauber */
    extern int sp_fumbleshield(struct castorder * co);
    extern int sp_shadowknights(struct castorder * co);
    extern int sp_combatrosthauch(struct castorder * co);
    extern int sp_kampfzauber(struct castorder * co);
    extern int sp_healing(struct castorder * co);
    extern int sp_keeploot(struct castorder * co);
    extern int sp_reanimate(struct castorder * co);
    extern int sp_chaosrow(struct castorder * co);
    extern int sp_flee(struct castorder * co);
    extern int sp_berserk(struct castorder * co);
    extern int sp_tiredsoldiers(struct castorder * co);
    extern int sp_reeling_arrows(struct castorder * co);
    extern int sp_denyattack(struct castorder * co);
    extern int sp_sleep(struct castorder * co);
    extern int sp_windshield(struct castorder * co);
    extern int sp_strong_wall(struct castorder * co);
    extern int sp_petrify(struct castorder * co);
    extern int sp_hero(struct castorder * co);
    extern int sp_frighten(struct castorder * co);
    extern int sp_mindblast(struct castorder * co);
    extern int sp_mindblast_temp(struct castorder * co);
    extern int sp_speed(struct castorder * co);
    extern int sp_wolfhowl(struct castorder * co);
    extern int sp_dragonodem(struct castorder * co);
    extern int sp_reduceshield(struct castorder * co);
    extern int sp_armorshield(struct castorder * co);
    extern int sp_stun(struct castorder * co);
    extern int sp_undeadhero(struct castorder * co);
    extern int sp_shadowcall(struct castorder * co);
    extern int sp_immolation(struct castorder * co);

#ifdef __cplusplus
}
#endif
#endif
