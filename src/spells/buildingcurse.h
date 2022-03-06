#pragma once
#ifndef _BCURSE_H
#define _BCURSE_H

extern const struct curse_type ct_magicwalls;
extern const struct curse_type ct_strongwall;
extern const struct curse_type ct_magicrunes;
extern const struct curse_type ct_nocostbuilding;

struct curse;
struct message;
enum objtype_t;

void register_buildingcurse(void);
struct message *cinfo_building(const void *obj, enum objtype_t typ, const struct curse * c, int self);

#endif                          /* _BCURSE_H */
