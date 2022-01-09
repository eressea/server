#ifndef H_KRNL_SKILL
#define H_KRNL_SKILL

#include "skill.h"

struct race;
struct unit;
struct region;
struct attrib;
struct attrib_type;

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct skill {
        enum skill_t id : 8;
        unsigned int level : 8;
        unsigned int weeks : 8;
        unsigned int old : 8;
    } skill;

    typedef int(*skillmod_fun) (const struct unit *, const struct region *,
            enum skill_t, int);
    typedef struct skillmod_data {
        enum skill_t skill;
        skillmod_fun special;
        double multiplier;
        int number;
        int bonus;
    } skillmod_data;

    extern struct attrib_type at_skillmod;

    int skillmod(const struct unit *u, const struct region *r,
            enum skill_t sk, int value);
    struct attrib *make_skillmod(enum skill_t sk, skillmod_fun special,
            double multiplier, int bonus);

    void increase_skill(struct unit * u, enum skill_t sk, unsigned int weeks);
    void reduce_skill(struct unit *u, skill * sv, unsigned int weeks);
    unsigned int skill_weeks(unsigned int level);
    int skill_compare(const skill * sk, const skill * sc);

    void sk_set(skill * sv, unsigned int level);

#ifdef __cplusplus
}
#endif
#endif
