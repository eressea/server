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

extern signed char skill_bonus(struct unit * u, struct region * r);

/* skillmod_data::flags -- wann gilt der modifier? */
#define SMF_ALWAYS     (1<<0) /* immer */
#define SMF_PRODUCTION (1<<1) /* für Produktion - am gebäude, an der einheit */
#define SMF_RIDING     (1<<2) /* Bonus für berittene - an der rasse*/

typedef struct skill {
#if SKILLPOINTS
	skill_t id;
	int value;
#else
	unsigned char id;
	unsigned char level;
	unsigned char learning;
#endif
} skill;

typedef struct skillmod_data {
	skill_t skill;
	int (*special)(const struct unit * u, const struct region * r, skill_t sk, int value);
	double multiplier;
	int bonus;
	int flags;
} skillmod_data;
extern attrib_type at_skillmod;
extern int rc_skillmod(const struct race * rc, const struct region *r, skill_t sk);
extern int skillmod(const attrib * a, const struct unit * u, const struct region * r, skill_t sk, int value, int flags);
extern void skill_init(void);
extern void skill_done(void);
extern struct attrib * make_skillmod(skill_t sk, unsigned int flags, int(*special)(const struct unit*, const struct region*, skill_t, int), double multiplier, int bonus);

extern const char * skillname(skill_t, const struct locale *);
extern skill_t sk_find(const char * name);

extern int level_days(int level);

#if SKILLPOINTS
# define skill_level(level) level_days(level)
#else
# define skill_level(level) (level)
extern void reduce_skill(skill * sv, int change);
extern int skill_compare(const skill * sk, const skill * sc);
#endif


#endif
