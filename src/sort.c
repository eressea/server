#include "sort.h"

#include "kernel/config.h"
#include "kernel/building.h"
#include "kernel/faction.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/region.h"
#include "kernel/ship.h"
#include "kernel/unit.h"

#include "util/keyword.h"
#include "util/param.h"
#include "util/parser.h"

static void sort_before(unit *v, unit **up) {
    unit *u = *up;
    region *r = u->region;
    unit **vp = &r->units;
    while (*vp != v)
        vp = &(*vp)->next;
    *vp = u;
    *up = u->next;
    u->next = v;
}

void do_sort(region *r)
{
    unit **up = &r->units;
    bool sorted = false;
    while (*up) {
        unit *u = *up;
        if (!fval(u, UFL_MARK) && !IS_PAUSED(u->faction)) {
            struct order *ord;
            for (ord = u->orders; ord; ord = ord->next) {
                if (getkeyword(ord) == K_SORT) {
                    char token[128];
                    const char *s;
                    param_t p;
                    int id;
                    unit *v;

                    init_order(ord, NULL);
                    s = gettoken(token, sizeof(token));
                    p = get_param(s, u->faction->locale);
                    id = getid();
                    v = findunit(id);

                    if (v == u) {
                        syntax_error(u, ord);
                    }
                    else if (!v || v->region != r) {
                        cmistake(u, ord, 258, MSG_EVENT);
                    }
                    else if (v->faction != u->faction && !IS_PAUSED(v->faction)) {
                        cmistake(u, ord, 258, MSG_EVENT);
                    }
                    else if (v->building != u->building || v->ship != u->ship) {
                        cmistake(u, ord, 259, MSG_EVENT);
                    }
                    else if (u->building && building_owner(u->building) == u) {
                        cmistake(u, ord, 260, MSG_EVENT);
                    }
                    else if (u->ship && ship_owner(u->ship) == u) {
                        cmistake(u, ord, 260, MSG_EVENT);
                    }
                    else {
                        switch (p) {
                        case P_AFTER:
                            *up = u->next;
                            u->next = v->next;
                            v->next = u;
                            fset(u, UFL_MARK);
                            sorted = true;
                            break;
                        case P_BEFORE:
                            if (v->ship && ship_owner(v->ship) == v) {
                                if (IS_PAUSED(v->faction)) {
                                    sort_before(v, up);
                                    ship_set_owner(u);
                                }
                                else {
                                    cmistake(u, ord, 261, MSG_EVENT);
                                    break;
                                }
                            }
                            else if (v->building && building_owner(v->building) == v) {
                                if (IS_PAUSED(v->faction)) {
                                    sort_before(v, up);
                                    building_set_owner(u);
                                }
                                else {
                                    cmistake(u, ord, 261, MSG_EVENT);
                                    break;
                                }
                            }
                            else {
                                sort_before(v, up);
                            }
                            fset(u, UFL_MARK);
                            sorted = true;
                            break;
                        default:
                            /* TODO: syntax error message? */
                            break;
                        }
                    }
                    break;
                }
            }
        }
        if (u == *up)
            up = &u->next;
    }
    if (sorted) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            freset(u, UFL_MARK);
        }
    }
}

void restack_units(void)
{
    region *r;
    for (r = regions; r; r = r->next) {
        do_sort(r);
    }
}

