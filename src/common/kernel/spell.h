/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_KRNL_SPELLS
#define H_KRNL_SPELLS
#ifdef __cplusplus
extern "C" {
#endif

struct fighter;
struct spell;
struct border_type;
struct attrib_type;
struct curse_type;
struct castorder;
struct curse;

/* Sprüche. Neue NUR hinten anfügen, oder das Datenfile geht kaputt */
enum {
	SPL_NOSPELL,
	SPL_ARTEFAKT_OF_POWER,
	SPL_ARTEFAKT_OF_AURAPOWER,
	SPL_ARTEFAKT_OF_REGENERATION,
	SPL_FIREBALL,
	SPL_HAGEL,
	SPL_RUSTWEAPON,
	SPL_COMBATRUST,
	SPL_TREEGROW,
	SPL_HEALING,
	SPL_HEALINGSONG,
	SPL_BADDREAMS,
	SPL_GOODDREAMS,
	SPL_DREAMREADING,
	SPL_SWEETDREAMS,
	SPL_TIREDSOLDIERS,
	SPL_PLAGUE,
	SPL_MAGICBOOST,
	SPL_CHAOSROW,
	SPL_SONG_OF_CONFUSION,
	SPL_FLEE,
	SPL_SONG_OF_FEAR,
	SPL_BERSERK,
	SPL_BLOODTHIRST,
	SPL_MAELSTROM,
	SPL_BLESSEDHARVEST,
	SPL_RAINDANCE,
	SPL_TRANSFERAURA_DRUIDE,
	SPL_TRANSFERAURA_BARDE,
	SPL_TRANSFERAURA_CHAOS,
	SPL_TRANSFERAURA_TRAUM,
	SPL_TRANSFERAURA_ASTRAL,
	SPL_STONEGOLEM,
	SPL_IRONGOLEM,
	SPL_SUMMONSHADOW,
	SPL_SUMMONSHADOWLORDS,
	SPL_REELING_ARROWS,
	SPL_ANTIMAGICZONE,
	SPL_CREATE_ANTIMAGICCRYSTAL,
	SPL_KAELTESCHUTZ,
	SPL_STEALAURA,
	SPL_SUMMONUNDEAD,
	SPL_AURALEAK,
	SPL_GREAT_DROUGHT,
	SPL_STRONG_WALL,
	SPL_HOMESTONE,
	SPL_DROUGHT,
	SPL_FOREST_FIRE,
	SPL_STRENGTH,
	SPL_SUMMONENT,
	SPL_DISTURBINGDREAMS,
	SPL_DENYATTACK,
	SPL_SLEEP,
	SPL_EARTHQUAKE,
	SPL_IRONKEEPER,
	SPL_STORMWINDS,
	SPL_GOODWINDS,
	SPL_FLYING_SHIP,
	SPL_SUMMON_ALP,
	SPL_WINDSHIELD,
	SPL_RAISEPEASANTS,
	SPL_DEPRESSION,
	SPL_HEADACHE,
	SPL_ARTEFAKT_NIMBLEFINGERRING,
	SPL_ENTERASTRAL,
	SPL_LEAVEASTRAL,
	SPL_SHOWASTRAL,
	SPL_VERSTEINERN,
	SPL_TREEWALKENTER,
	SPL_TREEWALKEXIT,
	SPL_CHAOSSUCTION,
	SPL_VIEWREALITY,
	SPL_DISRUPTASTRAL,
	SPL_SEDUCE,
	SPL_PUMP,
	SPL_CALM_MONSTER,
	SPL_HERO,
	SPL_FRIGHTEN,
	SPL_MINDBLAST,
	SPL_SPEED,
	SPL_SPEED2,
	SPL_FIREDRAGONODEM,
	SPL_DRAGONODEM,
	SPL_WYRMODEM,
	SPL_MAGICSTREET,
	SPL_REANIMATE,
	SPL_RECRUIT,
	SPL_GENEROUS,
	SPL_PERMTRANSFER,
	SPL_SONG_OF_PEACE,
	SPL_MIGRANT,
	SPL_RALLYPEASANTMOB,
	SPL_RAISEPEASANTMOB,
	SPL_ILL_SHAPESHIFT,
	SPL_WOLFHOWL,
	SPL_FOG_OF_CONFUSION,
	SPL_DREAM_OF_CONFUSION,
	SPL_RESISTMAGICBONUS,
	SPL_KEEPLOOT,
	SPL_SCHILDRUNEN,
	SPL_SONG_RESISTMAGIC,
	SPL_SONG_SUSCEPTMAGIC,
	SPL_ANALYSEMAGIC,
	SPL_ANALYSEDREAM,
	SPL_UNIT_ANALYSESONG,
	SPL_OBJ_ANALYSESONG,
	SPL_TYBIED_DESTROY_MAGIC,
	SPL_DESTROY_MAGIC,
	SPL_METEORRAIN,
	SPL_REDUCESHIELD,
	SPL_ARMORSHIELD,
	SPL_DEATHCLOUD,
	SPL_ORKDREAM,
	SPL_SUMMONDRAGON,
	SPL_READMIND,
	SPL_BABBLER,
	SPL_MOVECASTLE,
	SPL_BLESSSTONECIRCLE,
	SPL_ILLAUN_FAMILIAR,
	SPL_GWYRRD_FAMILIAR,
	SPL_DRAIG_FAMILIAR,
	SPL_CERDDOR_FAMILIAR,
	SPL_TYBIED_FAMILIAR,
	SPL_SONG_OF_ENSLAVE,
	SPL_TRUESEEING_GWYRRD,
	SPL_TRUESEEING_DRAIG,
	SPL_TRUESEEING_ILLAUN,
	SPL_TRUESEEING_CERDDOR,
	SPL_TRUESEEING_TYBIED,
	SPL_INVISIBILITY_GWYRRD,
	SPL_INVISIBILITY_DRAIG,
	SPL_INVISIBILITY_ILLAUN,
	SPL_INVISIBILITY_CERDDOR,
	SPL_INVISIBILITY_TYBIED,
	SPL_ARTEFAKT_CHASTITYBELT,
	SPL_ARTEFAKT_RUNESWORD,
	SPL_FUMBLECURSE,
	SPL_ICASTLE,
	SPL_GWYRRD_DESTROY_MAGIC,
	SPL_DRAIG_DESTROY_MAGIC,
	SPL_ILLAUN_DESTROY_MAGIC,
	SPL_CERDDOR_DESTROY_MAGIC,
	SPL_GWYRRD_ARMORSHIELD,
	SPL_DRAIG_FUMBLESHIELD,
	SPL_GWYRRD_FUMBLESHIELD,
	SPL_CERRDOR_FUMBLESHIELD,
	SPL_TYBIED_FUMBLESHIELD,
	SPL_SHADOWKNIGHTS,
	SPL_FIRESWORD,
	SPL_CREATE_TACTICCRYSTAL,
	SPL_ITEMCLOAK,
	SPL_FIREWALL,
	SPL_WISPS,
	SPL_SPARKLE_CHAOS,
	SPL_SPARKLE_DREAM,
	SPL_BAG_OF_HOLDING,
	SPL_PULLASTRAL,
	SPL_FETCHASTRAL,
	SPL_ILLAUN_EARN_SILVER,
	SPL_GWYRRD_EARN_SILVER,
	SPL_DRAIG_EARN_SILVER,
	SPL_TYBIED_EARN_SILVER,
	SPL_CERDDOR_EARN_SILVER,
	SPL_SHOCKWAVE,
	SPL_UNDEADHERO,
	SPL_ARTEFAKT_SACK_OF_CONSERVATION,
	SPL_BECOMEWYRM,
	SPL_ETERNIZEWALL,
	SPL_PUTTOREST,
	SPL_UNHOLYPOWER,
	SPL_HOLYGROUND,
	SPL_BLOODSACRIFICE,
	SPL_MALLORN,
	SPL_CLONECOPY,
	SPL_DRAINODEM,		/* 174? */
	SPL_AURA_OF_FEAR,	/* 175? */
	SPL_SHADOWCALL,		/* 176? */
	SPL_MALLORNTREEGROW,
	SPL_INVISIBILITY2_ILLAUN,
	SPL_BIGRECRUIT,
	MAXALLSPELLS,
	NO_SPELL = (spellid_t) -1
};

/* Prototypen */

void do_shock(struct unit *u, const char *reason);
int use_item_power(struct region * r, struct unit * u);
int use_item_regeneration(struct region * r, struct unit * u);
void showspells(struct region *r, struct unit *u);
int sp_antimagiczone(struct castorder *co);
int destr_curse(struct curse* c, int cast_level, int force);
	


/* Kampfzauber */
extern int sp_fumbleshield(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_shadowknights(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_combatrosthauch(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_kampfzauber(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_healing(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_keeploot(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_reanimate(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_chaosrow(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_flee(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_berserk(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_tiredsoldiers(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_reeling_arrows(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_denyattack(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_sleep(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_windshield(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_strong_wall(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_versteinern(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_hero(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_frighten(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_mindblast(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_speed(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_wolfhowl(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_dragonodem(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_reduceshield(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_armorshield(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_stun(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_undeadhero(struct fighter * fi, int level, int power, struct spell * sp);
extern int sp_shadowcall(struct fighter * fi, int level, int power, struct spell * sp);

/* ------------------------------------------------------------- */

#if USE_FIREWALL
/* für Feuerwände: in movement muß das noch explizit getestet werden.
 * besser wäre eine blcok_type::move() routine, die den effekt
 * der Bewegung auf eine struct unit anwendet.
 */
extern struct border_type bt_firewall;
extern struct border_type bt_wisps;
typedef struct wall_data {
	struct unit * mage;
	int force;
	boolean active;
} wall_data;
#endif

extern struct attrib_type at_cursewall;
extern struct attrib_type at_unitdissolve;
extern struct spell spelldaten[];

#ifdef __cplusplus
}
#endif
#endif
