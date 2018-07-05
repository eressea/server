#include <platform.h>

#include "kernel/faction.h"
#include "kernel/order.h"
#include "kernel/region.h"
#include "kernel/unit.h"

#include "util/log.h"

#include "automate.h"
#include "keyword.h"
#include "study.h"

#include <stdlib.h>

typedef struct student {
    unit *u;
    skill_t sk;
    int level;
} student;

#define MAXSTUDENTS 128

int cmp_students(const void *lhs, const void *rhs) {
    const student *a = (const student *)lhs;
    const student *b = (const student *)rhs;
    if (a->sk == b->sk) {
        /* sort by level, descending: */
        return b->level - a->level;
    }
    /* order by skill */
    return (int)a->sk - (int)b->sk;
}

void do_autostudy(region *r) {
    unit *u;
    int nstudents = 0;
    student students[MAXSTUDENTS];

    for (u = r->units; u; u = u->next) {
        keyword_t kwd = getkeyword(u->thisorder);
        if (kwd == K_AUTOSTUDY) {
            student * st = students + nstudents;
            if (++nstudents == MAXSTUDENTS) {
                log_fatal("you must increase MAXSTUDENTS");
            }
            st->u = u;
            init_order(u->thisorder, u->faction->locale);
            st->sk = getskill(u->faction->locale);
            st->level = effskill_study(u, st->sk);
        }
    }
    if (nstudents > 0) {
        int i, taught = 0;
        skill_t sk = NOSKILL;
        student *teacher = NULL, *student = NULL;

        qsort(students, nstudents, sizeof(student), cmp_students);
        for (i = 0; i != nstudents; ++i) {
            if (students[i].u) {
                if (sk == NOSKILL) {
                    sk = students[i].sk;
                }
                else if (sk != students[i].sk) {
                    continue;
                }
                students[i].u = NULL;
            }
        }
    }
}
