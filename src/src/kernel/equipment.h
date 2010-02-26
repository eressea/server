/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#ifndef H_KRNL_EQUIPMENT_H
#define H_KRNL_EQUIPMENT_H
#ifdef __cplusplus
extern "C" {
#endif
  struct spell;

  typedef struct itemdata {
    const struct item_type * itype;
    char * value;
    struct itemdata * next;
  } itemdata;

  typedef struct subsetitem {
    struct equipment * set;
    float chance;
  } subsetitem;

  typedef struct subset {
    float chance;
    subsetitem * sets;
  } subset;

  typedef struct equipment {
    char * name;
    struct itemdata * items;
    char * skills[MAXSKILLS];
    struct spell_list * spells;
    struct subset * subsets;
    struct equipment * next;
    void (*callback)(const struct equipment *, struct unit *);
  } equipment;


  extern struct equipment * create_equipment(const char * eqname);
  extern struct equipment * get_equipment(const char * eqname);

  extern void equipment_setitem(struct equipment * eq, const struct item_type * itype, const char * value);
  extern void equipment_setskill(struct equipment * eq, skill_t sk, const char * value);
  extern void equipment_addspell(struct equipment * eq, struct spell * sp);
  extern void equipment_setcallback(struct equipment * eq, void (*callback)(const struct equipment *, struct unit *));

  extern void equip_unit(struct unit * u, const struct equipment * eq);
  extern void equip_items(struct item ** items, const struct equipment * eq);

#ifdef __cplusplus
}
#endif
#endif
