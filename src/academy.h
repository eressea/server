#ifndef H_ACADEMY
#define H_ACADEMY

#include <skill.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    void academy_teaching_bonus(struct unit *u, skill_t sk, int academy);
    bool academy_can_teach(struct unit *teacher, struct unit *student, skill_t sk);
#ifdef __cplusplus
}
#endif
#endif
