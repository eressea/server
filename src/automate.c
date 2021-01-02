#include <platform.h>

#include "kernel/config.h"
#include "kernel/faction.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/region.h"
#include "kernel/unit.h"
#include "kernel/pool.h"
#include "kernel/item.h"

#include "util/keyword.h"
#include "util/log.h"

#include "automate.h"
#include "laws.h"
#include "study.h"

#include <stdlib.h>
#include <assert.h>

static int cmp_scholars(const void *lhs, const void *rhs) {
    const scholar *a = (const scholar *)lhs;
    const scholar *b = (const scholar *)rhs;
    int diff = b->level - a->level;
    return (diff != 0) ? diff : b->u->number - a->u->number;
}

int autostudy_init(scholar scholars[], int max_scholars, unit **units, skill_t *o_skill)
{
    unit *unext = NULL, *u = *units;
    faction *f = u->faction;
    int nscholars = 0;
    skill_t skill = NOSKILL;
    while (u) {
        if (!fval(u, UFL_MARK)) {
            keyword_t kwd = init_order(u->thisorder, u->faction->locale);
            if (kwd == K_AUTOSTUDY) {
                if (f == u->faction) {
                    unext = u->next;
                    if (long_order_allowed(u)) {
                        scholar * st = scholars + nscholars;
                        skill_t sk = getskill(u->faction->locale);
                        if (skill == NOSKILL && sk != NOSKILL) {
                            skill = sk;
                            if (o_skill) {
                                *o_skill = skill;
                            }
                        }
                        if (check_student(u, u->thisorder, sk)) {
                            if (sk == skill) {
                                fset(u, UFL_MARK);
                                st->level = (short)effskill_study(u, sk);
                                st->learn = 0;
                                st->u = u;
                                if (++nscholars >= max_scholars) {
                                    log_warning("you must increase MAXSCHOLARS");
                                    break;
                                }
                            }
                        }
                        else {
                            fset(u, UFL_MARK);
                        }
                    }
                }
            }
        }
        u = u->next;
    }
    while (unext && unext->faction != f) {
        unext = unext->next;
    }
    *units = unext;
    if (nscholars > 0) {
        qsort(scholars, nscholars, sizeof(scholar), cmp_scholars);
    }
    return nscholars;
}

static void teaching(scholar *s, int n) {
    assert(n <= s->u->number);
    // doppelter Effekt mit Lehrer:
    s->learn += n;
    s->u->flags |= UFL_LONGACTION;
}

static void learning(scholar *s, int n) {
    assert(n <= s->u->number);
    s->learn += n;
    s->u->flags |= UFL_LONGACTION;
}

void autostudy_run(scholar scholars[], int nscholars)
{
    int ti = 0;
    while (ti != nscholars) {
        int t, ts = 0, tt = 0, si = ti, mint = 0, ns = 0;
        for (t = ti; t != nscholars; ++t) {
            ts += scholars[t].u->number; /* count total scholars */
            mint = (ts + 10) / 11; /* need a minimum of ceil(ts/11) teachers */
            for (; mint > tt && si != nscholars; ++si) {
                tt += scholars[si].u->number;
            }
        }
        if (mint < tt) {
            /* die Einheit si-1 hat einen Mix aus Lehrer und Schüler */
            --si;
            ns = tt - mint;
        }
        /* now si splits the teachers and students 1:10 */
        /* first student must be 2 levels below first teacher: */
        for (; si != nscholars; ++si) {
            if (scholars[si].level + TEACHDIFFERENCE <= scholars[ti].level) {
                break;
            }
            ns = 0;
        }
        /* now si is the first unit we can teach, if we can teach any */
        if (si == nscholars) {
            /* there are no students, so standard learning for everyone */
            for (t = ti; t != nscholars; ++t) {
                learning(scholars + t, scholars[t].u->number);
            }
        }
        else {
            /* invariant: unit ti can still teach i students */
            int i = scholars[ti].u->number * STUDENTS_PER_TEACHER;
            /* invariant: unit si has n students that can still be taught */
            int s, n = (ns > 0) ? ns : scholars[si].u->number;
            for (t = ti, s = si; t != si && s != nscholars; ) {
                if (i >= n) {
                    /* t has more than enough teaching capacity for s */
                    i -= n;
                    teaching(scholars + s, n);
                    learning(scholars + s, scholars[s].u->number);
                    /* next student, please: */
                    if (++s == nscholars) {
                        /* no more students */
                        break;
                    }
                    n = scholars[s].u->number;
                }
                else {
                    /* a part of s gets credited and we need a new teacher: */
                    teaching(scholars + s, i);
                    /* we still need to teach n students in this unit: */
                    n -= i;
                    i = 0;
                    /* we want a new teacher for s. if any exists, it's next in the sequence. */
                    if (++t == si) {
                        continue;
                    }
                    if (scholars[t].level - TEACHDIFFERENCE < scholars[s].level) {
                        /* no remaining teacher can teach this student, so we skip ahead */
                        do {
                            /* remaining students learn without a teacher: */
                            learning(scholars + s, n);
                            if (++s == nscholars) {
                                break;
                            }
                            n = scholars[s].u->number;
                        } while (scholars[t].level - TEACHDIFFERENCE < scholars[s].level);
                    }
                    i = scholars[t].u->number * STUDENTS_PER_TEACHER;
                }
            }
            if (i > 0) {
                int remain = (STUDENTS_PER_TEACHER * scholars[t].u->number - i + STUDENTS_PER_TEACHER - 1) / STUDENTS_PER_TEACHER;
                /* teacher has remaining time */
                learning(scholars + t, remain);
            }
            ++t;
            for (; t < si; ++t) {
                /* teachers that did not teach */
                learning(scholars + t, scholars[t].u->number);
            }
            for (; s < nscholars; ++s) {
                /* students that were not taught */
                learning(scholars + s, scholars[s].u->number);
            }
        }
        ti = nscholars;
    }
}

void do_autostudy(region *r)
{
    static int config;
    static int batchsize = MAXSCHOLARS;
    static int max_scholars;
    scholar scholars[MAXSCHOLARS];
    unit *u;

    if (config_changed(&config)) {
        batchsize = config_get_int("automate.batchsize", MAXSCHOLARS);
        assert(batchsize <= MAXSCHOLARS);
    }
    for (u = r->units; u; u = u->next) {
        if (!fval(u, UFL_MARK)) {
            unit *ulist = u;
            int sum_scholars = 0;
            while (ulist) {
                skill_t skill = NOSKILL;
                int i, nscholars = autostudy_init(scholars, batchsize, &ulist, &skill);
                assert(ulist == NULL || ulist->faction == u->faction);
                sum_scholars += nscholars;
                if (sum_scholars > max_scholars) {
                    stats_count("automate.max_scholars", sum_scholars - max_scholars);
                    max_scholars = sum_scholars;
                }
                autostudy_run(scholars, nscholars);
                for (i = 0; i != nscholars; ++i) {
                    int days = STUDYDAYS * scholars[i].learn;
                    int money = learn_skill(scholars[i].u, skill, days, 0);
                    if (money > 0) {
                        use_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, money);
                        ADDMSG(&u->faction->msgs, msg_message("studycost",
                            "unit region cost skill", scholars[i].u, r, money, skill));
                    }
                }
            }
        }
        freset(u, UFL_MARK);
    }
}
