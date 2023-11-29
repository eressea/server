#pragma once
#ifndef H_KRNL_STUDY
#define H_KRNL_STUDY

#include <stdbool.h>

struct unit;
struct selist;
struct locale;
struct order;
enum skill_t;
enum magic_t;

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

bool can_teach(const struct unit* u);

enum magic_t getmagicskill(const struct locale *lang);
enum skill_t getskill(const struct locale *lang);
bool is_migrant(struct unit *u);
int study_cost(struct unit *u, enum skill_t sk);
bool check_student(const struct unit *u, struct order *ord,
        enum skill_t sk);

int learn_skill(struct unit *u, enum skill_t sk, int days,
        int studycost);
void change_skill_days(struct unit *u, enum skill_t sk, int days);

void produceexp(struct unit *u, enum skill_t sk);

void demon_skillchange(struct unit *u);

typedef void(*learn_fun)(struct unit *u, enum skill_t sk, int days);
void inject_learn(learn_fun fun);

#endif
