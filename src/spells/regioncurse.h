#ifndef _RCURSE_H
#define _RCURSE_H
#ifdef __cplusplus
extern "C" {
#endif

    struct region;
    struct curse_type;

    extern const struct curse_type ct_peacezone;
    extern const struct curse_type ct_drought;
    extern const struct curse_type ct_blessedharvest;
    extern const struct curse_type ct_godcursezone;
    extern const struct curse_type ct_gbdream;
    extern const struct curse_type ct_healing;
    extern const struct curse_type ct_antimagiczone;
    extern const struct curse_type ct_depression;
    extern const struct curse_type ct_astralblock;
    extern const struct curse_type ct_badmagicresistancezone;
    extern const struct curse_type ct_goodmagicresistancezone;
    extern const struct curse_type ct_holyground;
    extern const struct curse_type ct_fogtrap;
    extern const struct curse_type ct_magicstreet;
    extern const struct curse_type ct_maelstrom;
    extern const struct curse_type ct_riotzone;
    extern const struct curse_type ct_generous;
    extern const struct curse_type ct_badlearn;

    int harvest_effect(const struct region *r);
    void register_regioncurse(void);

#ifdef __cplusplus
}
#endif
#endif                          /* _RCURSE_H */
