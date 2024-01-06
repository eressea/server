#ifndef H_KRNL_SKILL
#define H_KRNL_SKILL

#include "skill.h"

#define MAX_WEEKS_TO_NEXT_LEVEL(level) ((level) * 2 + 1)
#define SKILL_DAYS_PER_WEEK 30
typedef struct skill {
    unsigned int id : 5;
	// max level 127
    unsigned int level : 7;
    unsigned int old : 7;
	// max 8191 days = 273 weeks = level 135
    unsigned int weeks : 13;
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

void increase_skill(struct unit * u, enum skill_t sk, unsigned int days);
void reduce_skill_weeks(struct unit *u, skill * sv, unsigned int weeks);
void reduce_skill(struct unit *u, skill *sv, unsigned int weeks);
int merge_skill(const skill* sv, const skill* sn, skill* result, int n, int add);
void sk_set_level(skill * sv, unsigned int level);
int skill_compare(const skill* sk, const skill* sc);

int skill_level(const struct skill * sv);
int skill_days(const struct skill * sv);
/** number of times the unit must STUDY to reach the next level: */
int skill_weeks(const struct skill * sv);


#define SK_SKILL(sv) ((skill_t) (sv->id))

#endif
