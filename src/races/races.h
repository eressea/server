#ifndef H_RACES
#define H_RACES

#ifdef __cplusplus
extern "C" {
#endif
    struct unit;

    void register_races(void);
    void equip_newunits(struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
