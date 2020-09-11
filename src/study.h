#ifndef H_KRNL_STUDY
#define H_KRNL_STUDY

#include "skill.h"
#include <kernel/types.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct selist;

#define STUDYDAYS 30
#define TEACHNUMBER 10
#define TEACHDIFFERENCE 2

    typedef struct teaching_info {
        struct selist *teachers;
        int students;
        int days;
    } teaching_info;

    extern const struct attrib_type at_learning;

    int teach_cmd(struct unit *u, struct order *ord);
    int study_cmd(struct unit *u, struct order *ord);

    magic_t getmagicskill(const struct locale *lang);
    skill_t getskill(const struct locale *lang);
    bool is_migrant(struct unit *u);
    int study_cost(struct unit *u, skill_t sk);
    bool check_student(const struct unit *u, struct order *ord, skill_t sk);

    typedef void(*learn_fun)(struct unit *u, skill_t sk, int days);

    int learn_skill(struct unit *u, skill_t sk, int days, int studycost);
    void change_skill_days(struct unit *u, skill_t sk, int days);

    void produceexp(struct unit *u, skill_t sk, int n);
    void produceexp_ex(struct unit *u, skill_t sk, int n, learn_fun learn);

    void demon_skillchange(struct unit *u);

    void inject_learn(learn_fun fun);

#ifdef __cplusplus
}
#endif
#endif
