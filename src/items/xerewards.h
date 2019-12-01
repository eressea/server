#ifndef H_ITM_XEREWARDS
#define H_ITM_XEREWARDS
#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct item_type;
    struct order;
    void register_xerewards(void);
    int use_skillpotion(struct unit *u, const struct item_type *itype, int amount, struct order *ord);
    int use_manacrystal(struct unit *u, const struct item_type *itype, int amount, struct order *ord);

#ifdef __cplusplus
}
#endif
#endif
