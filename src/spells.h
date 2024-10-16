#pragma once

struct castorder;
struct curse_type;
struct region;
struct unit;
struct message;

extern const struct curse_type ct_deathcloud;
void register_spells(void);
void init_spells(void);

#define ACTION_RESET      0x01  /* reset the one-time-flag FFL_SELECT (on first pass) */
#define ACTION_CANSEE     0x02  /* to people who can see the actor */
#define ACTION_CANNOTSEE  0x04  /* to people who can not see the actor */
int report_action(struct region *r, struct unit *actor, struct message *msg, int flags);

#define SHOWASTRAL_MAX_RADIUS 5
int sp_baddreams(struct castorder * co);
int sp_gooddreams(struct castorder * co);
int sp_viewreality(struct castorder * co);
int sp_showastral(struct castorder * co);
int sp_speed2(struct castorder* co);
int sp_goodwinds(struct castorder* co);
int sp_summonent(struct castorder* co);
int sp_summonundead(struct castorder *co);
int sp_maelstrom(struct castorder* co);
int sp_blessedharvest(struct castorder* co);
int sp_kaelteschutz(struct castorder* co);
int sp_treewalkenter(struct castorder* co);
int sp_treewalkexit(struct castorder* co);
int sp_holyground(struct castorder* co);
int sp_drought(struct castorder* co);
int sp_stormwinds(struct castorder* co);
int sp_fumblecurse(struct castorder* co);
int sp_deathcloud(struct castorder* co);
int sp_magicboost(struct castorder* co);
int sp_migranten(struct castorder* co);
int sp_great_drought(struct castorder *co);
int sp_magicstreet(struct castorder *co);
int sp_blessstonecircle(struct castorder *co);
int sp_destroy_magic(struct castorder *co);
int sp_rosthauch(struct castorder *co);
int sp_sparkle(struct castorder *co);
int sp_summon_familiar(struct castorder *co);

int sp_shadowdemons(struct castorder *co);
int sp_shadowlords(struct castorder *co);
int sp_analysemagic(struct castorder *co);
int sp_babbler(struct castorder *co);
int sp_charmingsong(struct castorder *co);
int sp_pump(struct castorder *co);
int sp_readmind(struct castorder *co);
int sp_auraleak(struct castorder *co);
int sp_movecastle(struct castorder *co);
