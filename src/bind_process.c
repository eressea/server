#include "bind_process.h"

#include <platform.h>
#include <kernel/alliance.h>
#include <kernel/config.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include "battle.h"
#include "economy.h"
#include "keyword.h"
#include "laws.h"
#include "magic.h"
#include "market.h"
#include "move.h"
#include "study.h"

#define PROC_LAND_REGION 0x0001
#define PROC_LONG_ORDER 0x0002

static void process_cmd(keyword_t kwd, int(*callback)(unit *, order *), int flags)
{
    region * r;
    for (r = regions; r; r = r->next) {
        unit * u;

        /* look for shortcuts */
        if (flags&PROC_LAND_REGION) {
            /* only execute when we are on solid terrain */
            while (r && (r->terrain->flags&LAND_REGION) == 0) {
                r = r->next;
            }
            if (!r) break;
        }

        for (u = r->units; u; u = u->next) {
            if (flags & PROC_LONG_ORDER) {
                if (kwd == getkeyword(u->thisorder)) {
                    callback(u, u->thisorder);
                }
            }
            else {
                order * ord;
                for (ord = u->orders; ord; ord = ord->next) {
                    if (kwd == getkeyword(ord)) {
                        callback(u, ord);
                    }
                }
            }
        }
    }
}

void process_produce(void) {
    struct region *r;
    for (r = regions; r; r = r->next) {
        unit * u;
        for (u = r->units; u; u = u->next) {
            order * ord;
            for (ord = u->orders; ord; ord = ord->next) {
                if (K_MAKE == getkeyword(ord)) {
                    make_cmd(u, ord);
                }
            }
        }
        produce(r);
        split_allocations(r);
    }
}

void process_battle(void) {
    do_battles();
}

void process_siege(void) {
    process_cmd(K_BESIEGE, siege_cmd, PROC_LAND_REGION);
}

void process_update_long_order(void) {
    region * r;
    for (r = regions; r; r = r->next) {
        unit * u;
        for (u = r->units; u; u = u->next) {
            update_long_order(u);
        }
    }
}

void process_markets(void) {
    do_markets();
}

void process_make_temp(void) {
    new_units();
}

void process_settings(void) {
    region * r;
    for (r = regions; r; r = r->next) {
        unit * u;
        for (u = r->units; u; u = u->next) {
            order * ord;
            for (ord = u->orders; ord; ord = ord->next) {
                keyword_t kwd = getkeyword(ord);
                if (kwd == K_BANNER) {
                    banner_cmd(u, ord);
                }
                else if (kwd == K_EMAIL) {
                    email_cmd(u, ord);
                }
                else if (kwd == K_SEND) {
                    send_cmd(u, ord);
                }
                else if (kwd == K_PASSWORD) {
                    password_cmd(u, ord);
                }
            }
        }
    }
}

void process_ally(void) {
    process_cmd(K_ALLY, ally_cmd, 0);
}

void process_prefix(void) {
    process_cmd(K_PREFIX, prefix_cmd, 0);
}

void process_setstealth(void) {
    process_cmd(K_SETSTEALTH, setstealth_cmd, 0);
}

void process_status(void) {
    process_cmd(K_STATUS, status_cmd, 0);
}

void process_name(void) {
    process_cmd(K_NAME, name_cmd, 0);
    process_cmd(K_DISPLAY, display_cmd, 0);
}

void process_group(void) {
    process_cmd(K_GROUP, group_cmd, 0);
}

void process_origin(void) {
    process_cmd(K_URSPRUNG, origin_cmd, 0);
}

void process_quit(void) {
    process_cmd(K_QUIT, quit_cmd, 0);
    quit();
}

void process_study(void) {
    process_cmd(K_TEACH, teach_cmd, PROC_LONG_ORDER);
    process_cmd(K_STUDY, study_cmd, PROC_LONG_ORDER);
}

void process_movement(void) {
    region *r;

    movement();
    for (r = regions; r; r = r->next) {
        if (r->ships) {
            sinkships(r);
        }
    }
}

void process_use(void) {
    process_cmd(K_USE, use_cmd, 0);
}

void process_leave(void) {
    process_cmd(K_LEAVE, leave_cmd, 0);
}

void process_promote(void) {
    process_cmd(K_PROMOTION, promotion_cmd, 0);
}

void process_renumber(void) {
    process_cmd(K_NUMBER, renumber_cmd, 0);
    renumber_factions();
}

void process_restack(void) {
    restack_units();
}

void process_setspells(void) {
    process_cmd(K_COMBATSPELL, combatspell_cmd, 0);
}

void process_sethelp(void) {
    process_cmd(K_ALLY, ally_cmd, 0);
}

void process_contact(void) {
    process_cmd(K_CONTACT, contact_cmd, 0);
}

void process_magic(void) {
    magic();
}

void process_give_control(void) {
    process_cmd(K_CONTACT, give_control_cmd, 0);
}

void process_guard_on(void) {
    process_cmd(K_GUARD, guard_on_cmd, PROC_LAND_REGION);
}

void process_explain(void) {
    process_cmd(K_RESHOW, reshow_cmd, 0);
}

void process_reserve(void) {
    int rule = config_get_int("rules.reserve.twophase", 0);
    if (rule) {
        process_cmd(K_RESERVE, reserve_self, 0);
    }
    process_cmd(K_RESERVE, reserve_cmd, 0);
}

void process_claim(void) {
    process_cmd(K_CLAIM, claim_cmd, 0);
}

void process_follow(void) {
    struct region *r;
    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            follow_unit(u);
        }
    }
}

void process_messages(void) {
    process_cmd(K_MAIL, mail_cmd, 0);
}

void process_guard_off(void) {
    process_cmd(K_GUARD, guard_off_cmd, PROC_LAND_REGION);
}

void process_regeneration(void) {
    monthly_healing();
    regenerate_aura();
}

void process_enter(int final) {
    region *r;
    for (r = regions; r; r = r->next) {
        do_enter(r, final);
    }
}

void process_maintenance(void) {
    region *r;
    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            order *ord;
            for (ord = u->orders; ord; ord = ord->next) {
                keyword_t kwd = getkeyword(ord);
                if (kwd == K_PAY) {
                    pay_cmd(u, ord);
                }
            }
        }
        maintain_buildings(r, 0);
    }
}

void process_alliance(void) {
    alliance_cmd();
}

void process_idle(void) {
    region *r;
    for (r = regions; r; r = r->next) {
        auto_work(r);
    }
}

void process_set_default(void) {
    if (!keyword_disabled(K_DEFAULT)) {
        defaultorders();
    }
}

