/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#ifndef H_KRNL_SKILL
#define H_KRNL_SKILL

#include <skill.h>

struct race;
struct unit;
struct region;
struct attrib;
struct attrib_type;

#ifdef __cplusplus
extern "C" {
#endif

    /* skillmod_data::flags -- wann gilt der modifier? */
#define SMF_ALWAYS     (1<<0)   /* immer */
#define SMF_PRODUCTION (1<<1)   /* für Produktion - am gebäude, an der einheit */
#define SMF_RIDING     (1<<2)   /* Bonus für berittene - an der rasse */

    typedef struct skill {
#ifdef LOMEM
        int id:8;
        unsigned int level:8;
        unsigned int weeks:8;
        unsigned int old:8;
#else
        int id;
        int level;
        int weeks;
        int old;
#endif
    } skill;

    typedef int(*skillmod_fun) (const struct unit *, const struct region *,
        skill_t, int);
    typedef struct skillmod_data {
        skill_t skill;
        skillmod_fun special;
        double multiplier;
        int number;
        int bonus;
        int flags;
    } skillmod_data;
    extern struct attrib_type at_skillmod;
    extern int rc_skillmod(const struct race *rc, const struct region *r,
        skill_t sk);
    extern int skillmod(const struct attrib *a, const struct unit *u,
        const struct region *r, skill_t sk, int value, int flags);
    extern struct attrib *make_skillmod(skill_t sk, unsigned int flags,
        skillmod_fun special, double multiplier, int bonus);

    extern int level_days(int level);
    extern int level(int days);

#define skill_level(level) (level)
    extern void reduce_skill(struct unit *u, skill * sv, unsigned int change);
    extern int skill_weeks(int level);
    extern int skill_compare(const skill * sk, const skill * sc);

    extern void sk_set(skill * sv, int level);

#ifdef __cplusplus
}
#endif
#endif
