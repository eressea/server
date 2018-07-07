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

int autostudy_init(student students[], int max_students, region *r)
{
    unit *u;
    int nstudents = 0;

    for (u = r->units; u; u = u->next) {
        keyword_t kwd = getkeyword(u->thisorder);
        if (kwd == K_AUTOSTUDY) {
            student * st = students + nstudents;
            if (++nstudents == max_students) {
                log_fatal("you must increase MAXSTUDENTS");
            }
            st->u = u;
            init_order(u->thisorder, u->faction->locale);
            st->sk = getskill(u->faction->locale);
            st->level = effskill_study(u, st->sk);
            st->learn = 0;
        }
    }
    if (nstudents > 0) {
        qsort(students, nstudents, sizeof(student), cmp_students);
    }
    return nstudents;
}

#define MAXSTUDENTS 128

void do_autostudy(region *r) {
    student students[MAXSTUDENTS];
    int nstudents = autostudy_init(students, MAXSTUDENTS, r);

    if (nstudents > 0) {
        int i;
        skill_t sk = NOSKILL;

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
