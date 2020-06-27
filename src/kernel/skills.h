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
        int id : 8;
        int level : 8;
        int weeks : 8;
        int old : 8;
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
