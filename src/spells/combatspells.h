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

    struct castorder;

    /* Kampfzauber */
    int sp_fumbleshield(struct castorder * co);
    int sp_shadowknights(struct castorder * co);
    int sp_combatrosthauch(struct castorder * co);
    int sp_healing(struct castorder * co);
    int sp_keeploot(struct castorder * co);
    int sp_reanimate(struct castorder * co);
    int sp_chaosrow(struct castorder * co);
    int sp_berserk(struct castorder * co);
    int sp_tiredsoldiers(struct castorder * co);
    int sp_reeling_arrows(struct castorder * co);
    int sp_appeasement(struct castorder * co);
    int sp_sleep(struct castorder * co);
    int sp_windshield(struct castorder * co);
    int sp_strong_wall(struct castorder * co);
    int sp_petrify(struct castorder * co);
    int sp_hero(struct castorder * co);
    int sp_frighten(struct castorder * co);
    int sp_mindblast(struct castorder * co);
    int sp_speed(struct castorder * co);
    int sp_wolfhowl(struct castorder * co);
    int sp_igjarjuk(struct castorder * co);
    int sp_dragonodem(struct castorder * co);
    int sp_reduceshield(struct castorder * co);
    int sp_armorshield(struct castorder * co);
    int sp_stun(struct castorder * co);
    int sp_undeadhero(struct castorder * co);
    int sp_immolation(struct castorder * co);

    int flee_spell(struct castorder * co, int strength);
    int damage_spell(struct castorder * co, int dmg, int strength);
    int armor_spell(struct castorder * co, int per_level, int time_multi);

#ifdef __cplusplus
}
#endif
#endif
