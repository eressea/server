#ifndef H_KRNL_SPY
#define H_KRNL_SPY
#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct region;
    struct strlist;
    struct order;
    struct faction;
    struct ship;

    int setstealth_cmd(struct unit *u, struct order *ord);
    int spy_cmd(struct unit *u, struct order *ord);
    int sabotage_cmd(struct unit *u, struct order *ord);
    void spy_message(int spy, const struct unit *u,
        const struct unit *target);
    void set_factionstealth(struct unit * u, struct faction * f);
    void sink_ship(struct ship * sh);

#ifdef __cplusplus
}
#endif
#endif
