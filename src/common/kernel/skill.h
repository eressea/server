/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef SKILL_H
#define SKILL_H

typedef struct modifiers {
	signed char value[MAXSKILLS];
} modifiers;

typedef struct skillmods {
	modifiers mod[MAXTERRAINS];
} skillmods;

extern skillmods * skill_refresh(struct unit * u);
extern signed char skill_bonus(struct unit * u, struct region * r);
char eff_skill(const struct unit * u, skill_t sk, const struct region * r);

int pure_skill(struct unit * u, skill_t sk, struct region * r);

/* skillmod_data::flags -- wann gilt der modifier? */
#define SMF_ALWAYS     (1<<0) /* immer */
#define SMF_PRODUCTION (1<<1) /* für Produktion - am gebäude, an der einheit */
#define SMF_RIDING     (1<<2) /* Bonus für berittene - an der rasse*/

typedef struct skillmod_data {
	skill_t skill;
	int (*special)(const struct unit * u, const struct region * r, skill_t skill, int value);
	double multiplier;
	int bonus;
	int flags;
} skillmod_data;
extern attrib_type at_skillmod;
extern int skillmod(const attrib * a, const struct unit * u, const struct region * r, skill_t sk, int value, int flags);
extern void skill_init(void);
extern void skill_done(void);
extern struct attrib * make_skillmod(skill_t skill, unsigned int flags, int(*special)(const struct unit*, const struct region*, skill_t, int), double multiplier, int bonus);

int level_days(int level);
void remove_zero_skills(void);

#endif
