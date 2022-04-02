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

enum magic_t getmagicskill(const struct locale *lang);
enum skill_t getskill(const struct locale *lang);
bool is_migrant(struct unit *u);
int study_cost(struct unit *u, enum skill_t sk);
bool check_student(const struct unit *u, struct order *ord,
        enum skill_t sk);

typedef void(*learn_fun)(struct unit *u, enum skill_t sk, int days);

int learn_skill(struct unit *u, enum skill_t sk, int days,
        int studycost);
void change_skill_days(struct unit *u, enum skill_t sk, int days);

void produceexp(struct unit *u, enum skill_t sk, int n);
void produceexp_ex(struct unit *u, enum skill_t sk, int n,
        learn_fun learn);

void demon_skillchange(struct unit *u);

void inject_learn(learn_fun fun);

#endif
