/* vi: set ts=2:
 *
 *	$Id: skill.h,v 1.2 2001/01/26 16:19:40 enno Exp $
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
#define SMF_ALWAYS     0x01 /* immer */
#define SMF_PRODUCTION 0x02 /* für Produktion */

typedef struct skillmod_data {
	int (*special)(const struct unit * u, const struct region * r, skill_t skill, int value);
	int multiplier;
	int bonus;
	int flags;
} skillmod_data;
extern attrib_type at_skillmod;
extern int skillmod(const attrib * a, const struct unit * u, const struct region * r, skill_t sk, int value, int flags);
extern void skill_init(void);
extern void skill_done(void);

int level_days(int level);
void remove_zero_skills(void);

#endif
