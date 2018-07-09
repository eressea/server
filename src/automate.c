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

int cmp_scholars(const void *lhs, const void *rhs)
{
    const scholar *a = (const scholar *)lhs;
    const scholar *b = (const scholar *)rhs;
    if (a->sk == b->sk) {
        /* sort by level, descending: */
        return b->level - a->level;
    }
    /* order by skill */
    return (int)a->sk - (int)b->sk;
}

int autostudy_init(scholar scholars[], int max_scholars, region *r)
{
    unit *u;
    int nscholars = 0;

    for (u = r->units; u; u = u->next) {
        keyword_t kwd = getkeyword(u->thisorder);
        if (kwd == K_AUTOSTUDY) {
            scholar * st = scholars + nscholars;
            if (++nscholars == max_scholars) {
                log_fatal("you must increase MAXSCHOLARS");
            }
            st->u = u;
            init_order(u->thisorder, u->faction->locale);
            st->sk = getskill(u->faction->locale);
            st->level = effskill_study(u, st->sk);
            st->learn = 0;
        }
    }
    if (nscholars > 0) {
        qsort(scholars, nscholars, sizeof(scholar), cmp_scholars);
    }
    return nscholars;
}

void autostudy_run(scholar scholars[], int nscholars)
{
    int i, t, s, ti = 0, si = 0, ts = 0, tt = 0;
    skill_t sk = scholars[0].sk;
    for (i = ti; i != nscholars && scholars[i].sk == sk; ++i) {
        int mint;
        ts += scholars[i].u->number; /* count total scholars */
        mint = (ts + 10) / 11; /* need a minimum of ceil(ts/11) teachers */
        while (mint>tt) {
            tt += scholars[si++].u->number;
        }
    }
    /* now si splits the teachers and students 1:10 */
    /* first student must be 2 levels below first teacher: */
    while (scholars[ti].level - TEACHDIFFERENCE > scholars[si].level) {
        tt += scholars[si++].u->number;
    }
    /* invariant: unit ti can still teach i students */
    i = scholars[ti].u->number * 10;
    for (t = ti, s = si; t != si && s != nscholars; ++t) {
        /* TODO: is there no constant for students per teacher? */
        while (s != nscholars) {
            int n = scholars[s].u->number;
            scholars[s].learn += n;
            if (i >= n) {
                i -= n;
                scholars[s].learn += n;
                /* next student */
            }
            else {
                scholars[s].learn += i;
                /* go to next suitable teacher */
                do {
                    ++t;
                }
                while (scholars[t].level - TEACHDIFFERENCE < scholars[s].level);
            }
        }
    }
}

#define MAXSCHOLARS 128

void do_autostudy(region *r)
{
    scholar scholars[MAXSCHOLARS];
    int nscholars = autostudy_init(scholars, MAXSCHOLARS, r);

    if (nscholars > 0) {
        int i;
        skill_t sk = NOSKILL;

        for (i = 0; i != nscholars; ++i) {
            if (scholars[i].u) {
                if (sk == NOSKILL) {
                    sk = scholars[i].sk;
                }
                else if (sk != scholars[i].sk) {
                    continue;
                }
                scholars[i].u = NULL;
            }
        }
    }
}
