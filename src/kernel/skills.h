#pragma once

#include "skill.h"

#define MAX_WEEKS_TO_NEXT_LEVEL(level) ((level) * 2 + 1)
#define MAX_DAYS_TO_NEXT_LEVEL(level) (SKILL_DAYS_PER_WEEK * MAX_WEEKS_TO_NEXT_LEVEL(level))

typedef struct skill {
    unsigned int id : 5;
    unsigned int level : 7;
    unsigned int old : 7;
    unsigned int days : 13;
} skill;

struct race;
struct unit;
struct region;
struct attrib;

typedef int(*skillmod_fun) (const struct unit*, const struct region*,
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

void increase_skill_weeks(struct unit * u, enum skill_t sk, const unsigned int weeks);
void reduce_skill_weeks(struct unit *u, skill * sv, const unsigned int weeks);
int merge_skill(const skill* sv, const skill* sn, skill* result, int n, int add);
void sk_set_level(skill * sv, int level);
int skill_compare(const skill* sk, const skill* sc);

int skill_level(struct unit *u, enum skill_t sk);
/** number of days-equivalent the unit must STUDY to reach the next level: */
int skill_days(struct unit *u, enum skill_t sk);

#define SK_SKILL(sv) ((skill_t) (sv->id))
