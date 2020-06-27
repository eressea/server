#ifndef H_RACES
#define H_RACES

#ifdef __cplusplus
extern "C" {
#endif
    struct unit;

    void register_races(void);
    void make_undead_unit(struct unit *);
    void equip_newunits(struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
