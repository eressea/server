#ifndef H_SPL_SPELLS
#define H_SPL_SPELLS

#ifdef __cplusplus
extern "C" {
#endif

    struct castorder;
    struct curse_type;
    struct region;
    struct unit;
    struct message;

    void register_spells(void);

#define SHOWASTRAL_MAX_RADIUS 5
    int sp_baddreams(struct castorder * co);
    int sp_gooddreams(struct castorder * co);
    int sp_viewreality(struct castorder * co);
    int sp_showastral(struct castorder * co);
    int sp_speed2(struct castorder* co);
    int sp_goodwinds(struct castorder* co);
    int sp_summonent(struct castorder* co);
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

#define ACTION_RESET      0x01  /* reset the one-time-flag FFL_SELECT (on first pass) */
#define ACTION_CANSEE     0x02  /* to people who can see the actor */
#define ACTION_CANNOTSEE  0x04  /* to people who can not see the actor */
    int report_action(struct region *r, struct unit *actor, struct message *msg, int flags);

#ifdef __cplusplus
}
#endif
#endif
