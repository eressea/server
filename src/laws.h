/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef H_GC_LAWS
#define H_GC_LAWS

#include <kernel/types.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct region;
    struct building;
    struct faction;
    struct order;
    struct attrib_type;

    int writepasswd(void);
    void demographics(void);
    void immigration(void);
    void update_guards(void);
    void update_subscriptions(void);
    void deliverMail(struct faction *f, struct region *r, struct unit *u,
        const char *s, struct unit *receiver);

    bool renamed_building(const struct building * b);
    int rename_building(struct unit * u, struct order * ord, struct building * b, const char *name);
    void get_food(struct region * r);
    int can_contact(const struct region *r, const struct unit *u, const struct unit *u2);

    int enter_building(struct unit *u, struct order *ord, int id, bool report);
    int enter_ship(struct unit *u, struct order *ord, int id, bool report);

    /* eressea-specific. put somewhere else, please. */
    void processorders(void);
    extern struct attrib_type at_germs;

    extern int dropouts[2];
    extern int *age;


    extern void new_units(void);
    extern void defaultorders(void);
    extern void quit(void);
    extern void monthly_healing(void);
    extern void renumber_factions(void);
    extern void restack_units(void);
    extern void update_long_order(struct unit *u);
    extern void sinkships(struct region * r);
    extern void do_enter(struct region *r, bool is_final_attempt);

    extern int password_cmd(struct unit *u, struct order *ord);
    extern int banner_cmd(struct unit *u, struct order *ord);
    extern int email_cmd(struct unit *u, struct order *ord);
    extern int send_cmd(struct unit *u, struct order *ord);
    extern int ally_cmd(struct unit* u, struct order *ord);
    extern int prefix_cmd(struct unit *u, struct order *ord);
    extern int setstealth_cmd(struct unit *u, struct order *ord);
    extern int status_cmd(struct unit *u, struct order *ord);
    extern int display_cmd(struct unit *u, struct order *ord);
    extern int group_cmd(struct unit *u, struct order *ord);
    extern int origin_cmd(struct unit *u, struct order *ord);
    extern int quit_cmd(struct unit *u, struct order *ord);
    extern int name_cmd(struct unit *u, struct order *ord);
    extern int use_cmd(struct unit *u, struct order *ord);
    extern int siege_cmd(struct unit *u, struct order *ord);
    extern int leave_cmd(struct unit *u, struct order *ord);
    extern int pay_cmd(struct unit *u, struct order *ord);
    extern int promotion_cmd(struct unit *u, struct order *ord);
    extern int renumber_cmd(struct unit *u, struct order *ord);
    extern int combatspell_cmd(struct unit *u, struct order *ord);
    extern int contact_cmd(struct unit *u, struct order *ord);
    extern int guard_on_cmd(struct unit *u, struct order *ord);
    extern int guard_off_cmd(struct unit *u, struct order *ord);
    extern int reshow_cmd(struct unit *u, struct order *ord);
    extern int mail_cmd(struct unit *u, struct order *ord);
    extern int reserve_cmd(struct unit *u, struct order *ord);
    extern int reserve_self(struct unit *u, struct order *ord);
    extern int claim_cmd(struct unit *u, struct order *ord);
    extern int follow_cmd(struct unit *u, struct order *ord);

    bool cansee(const struct faction *f, const struct region *r,
        const struct unit *u, int modifier);
    bool cansee_durchgezogen(const struct faction *f, const struct region *r,
        const struct unit *u, int modifier);
    bool cansee_unit(const struct unit *u, const struct unit *target,
        int modifier);
    bool seefaction(const struct faction *f, const struct region *r,
        const struct unit *u, int modifier);
    int armedmen(const struct unit *u, bool siege_weapons);
    int peasant_luck_effect(int peasants, int luck, int maxp, double variance);

    #define FORCE_LEAVE_POSTCOMBAT 1
    #define FORCE_LEAVE_ALL 2
    bool rule_force_leave(int flag);
    void force_leave(struct region *r);

#ifdef __cplusplus
}
#endif
#endif
