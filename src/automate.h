#pragma once

#ifndef H_GC_AUTOMATE
#define H_GC_AUTOMATE

#include "skill.h"

struct region;
struct unit;

typedef struct scholar {
    struct unit *u;
    int learn;
    short level;
} scholar;

#define MAXSCHOLARS 128
#define STUDENTS_PER_TEACHER 10

void do_autostudy(struct region *r);

int autostudy_init(scholar scholars[], int max_scholars, struct unit **units, skill_t *o_skill);
void autostudy_run(scholar scholars[], int nscholars);

#endif
