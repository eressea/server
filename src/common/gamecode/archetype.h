/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2007   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#ifndef H_GC_ARCHETYPE
#define H_GC_ARCHETYPE

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct rule {
    boolean allow;
    char * property;
    char * value;
  } rule;

  typedef struct archetype {
    struct archetype * next;
    char * name[2];
    int size;
    struct building_type * btype;
    struct equipment * equip;
    struct construction * ctype;
    struct rule * rules;
  } archetype;

  extern const struct archetype * find_archetype(const char * s, const struct locale * lang);
  extern void init_archetypes(void);
  extern void register_archetype(struct archetype * arch);
  extern void register_archetypes(void);

  extern struct attrib_type at_recruit;

#ifdef __cplusplus
}
#endif

#endif
