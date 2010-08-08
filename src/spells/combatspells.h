/* vi: set ts=2:
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
  extern int sp_fumbleshield(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_shadowknights(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_combatrosthauch(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_kampfzauber(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_healing(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_keeploot(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_reanimate(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_chaosrow(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_flee(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_berserk(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_tiredsoldiers(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_reeling_arrows(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_denyattack(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_sleep(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_windshield(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_strong_wall(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_petrify(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_hero(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_frighten(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_mindblast(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_mindblast_temp(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_speed(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_wolfhowl(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_dragonodem(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_reduceshield(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_armorshield(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_stun(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_undeadhero(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_shadowcall(struct fighter * fi, int level, double power, struct spell * sp);
  extern int sp_immolation(struct fighter * fi, int level, double power, struct spell * sp);

#ifdef __cplusplus
}
#endif
#endif
