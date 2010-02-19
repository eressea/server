/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_KRNL_STUDY
#define H_KRNL_STUDY

#ifdef __cplusplus
extern "C" {
#endif

extern int teach_cmd(struct unit * u, struct order * ord);
extern int learn_cmd(struct unit * u, struct order * ord);

extern magic_t getmagicskill(const struct locale * lang);
extern boolean is_migrant(struct unit *u);
extern int study_cost(struct unit *u, skill_t talent);

#define MAXTEACHERS 4
typedef struct teaching_info {
  struct unit * teachers[MAXTEACHERS];
  int value;
} teaching_info;

extern const struct attrib_type at_learning;

#ifdef __cplusplus
}
#endif

#endif
