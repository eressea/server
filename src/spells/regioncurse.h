#pragma once
#ifndef _RCURSE_H
#define _RCURSE_H

extern const struct curse_type ct_peacezone;
extern const struct curse_type ct_drought;
extern const struct curse_type ct_blessedharvest;
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

struct region;

int harvest_effect(const struct region *r);
void register_regioncurse(void);

#endif                          /* _RCURSE_H */
