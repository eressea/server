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

void restack_units(void)
{
    region *r;
    for (r = regions; r; r = r->next) {
        unit **up = &r->units;
        bool sorted = false;
        while (*up) {
            unit *u = *up;
            if (!fval(u, UFL_MARK)) {
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
                        p = findparam(s, u->faction->locale);
                        id = getid();
                        v = findunit(id);

                        if (!v || v->faction != u->faction || v->region != r) {
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
                        else if (v == u) {
                            syntax_error(u, ord);
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
                                    cmistake(v, ord, 261, MSG_EVENT);
                                }
                                else if (v->building && building_owner(v->building) == v) {
                                    cmistake(v, ord, 261, MSG_EVENT);
                                }
                                else {
                                    unit **vp = &r->units;
                                    while (*vp != v)
                                        vp = &(*vp)->next;
                                    *vp = u;
                                    *up = u->next;
                                    u->next = v;
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
}

