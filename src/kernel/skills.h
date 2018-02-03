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

    typedef struct skill {
        skill_t id : 16;
        int level : 16;
        int weeks : 16;
        int old : 16;
    } skill;

    typedef int(*skillmod_fun) (const struct unit *, const struct region *,
        skill_t, int);
    typedef struct skillmod_data {
        skill_t skill;
        skillmod_fun special;
        double multiplier;
        int number;
        int bonus;
    } skillmod_data;

    extern struct attrib_type at_skillmod;

    int rc_skillmod(const struct race *rc, const struct region *r, skill_t sk);
    int skillmod(const struct unit *u, const struct region *r, skill_t sk, int value);
    struct attrib *make_skillmod(skill_t sk, skillmod_fun special, double multiplier, int bonus);

    int level_days(int level);
    int level(int days);

#define skill_level(level) (level)
    void increase_skill(struct unit * u, skill_t sk, int weeks);
    void reduce_skill(struct unit *u, skill * sv, int weeks);
    int skill_weeks(int level);
    int skill_compare(const skill * sk, const skill * sc);

    void sk_set(skill * sv, int level);

#ifdef __cplusplus
}
#endif
#endif
