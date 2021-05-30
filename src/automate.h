#pragma once

#ifndef H_GC_AUTOMATE
#define H_GC_AUTOMATE

struct region;
struct unit;
enum skill_t;

typedef struct scholar {
    struct unit *u;
    int learn;
    short level;
} scholar;

#define MAXSCHOLARS 128
#define STUDENTS_PER_TEACHER 10

void do_autostudy(struct region *r);

int autostudy_init(scholar scholars[], int max_scholars,
        struct unit **units, enum skill_t *o_skill);
void autostudy_run(scholar scholars[], int nscholars);

#endif
