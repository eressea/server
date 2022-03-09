#ifndef H_GC_LAWS
#define H_GC_LAWS

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
    enum param_t;

    struct locale;
    struct unit;
    struct region;
    struct building;
    struct faction;
    struct order;
    struct attrib_type;

    extern struct attrib_type at_germs;

#define MAXNEWPLAYERS 4
    extern int newbies[MAXNEWPLAYERS];
    extern int dropouts[2];

    void demographics(void);
    void immigration(void);
    void update_guards(void);
    void deliverMail(struct faction *f, struct region *r, struct unit *u,
        const char *s, struct unit *receiver);

    bool renamed_building(const struct building * b);
    int rename_building(struct unit * u, struct order * ord, struct building * b, const char *name);
    void get_food(struct region * r);

    int enter_building(struct unit *u, struct order *ord, int id, bool report);
    int enter_ship(struct unit *u, struct order *ord, int id, bool report);

    void turn_begin(void);
    void turn_process(void);
    void turn_end(void);

    int checkunitnumber(const struct faction * f, int add);
    void new_units(void);
    void defaultorders(void);
    void quit(void);
    void monthly_healing(void);
    void update_long_order(struct unit *u);
    void sinkships(struct region * r);
    void do_enter(struct region *r, bool is_final_attempt);
    bool long_order_allowed(const struct unit *u, bool flags_only);
    bool password_wellformed(const char *password);

    int expel_cmd(struct unit *u, struct order *ord);
    int locale_cmd(struct unit *u, struct order *ord);
    int password_cmd(struct unit *u, struct order *ord);
    int banner_cmd(struct unit *u, struct order *ord);
    int email_cmd(struct unit *u, struct order *ord);
    int send_cmd(struct unit *u, struct order *ord);
    int ally_cmd(struct unit* u, struct order *ord);
    int prefix_cmd(struct unit *u, struct order *ord);
    int setstealth_cmd(struct unit *u, struct order *ord);
    int status_cmd(struct unit *u, struct order *ord);
    int display_cmd(struct unit *u, struct order *ord);
    int group_cmd(struct unit *u, struct order *ord);
    int origin_cmd(struct unit *u, struct order *ord);
    int quit_cmd(struct unit *u, struct order *ord);
    int name_cmd(struct unit *u, struct order *ord);
    int use_cmd(struct unit *u, struct order *ord);
    int leave_cmd(struct unit *u, struct order *ord);
    int pay_cmd(struct unit *u, struct order *ord);
    int promotion_cmd(struct unit *u, struct order *ord);
    int combatspell_cmd(struct unit *u, struct order *ord);
    int contact_cmd(struct unit *u, struct order *ord);
    int guard_on_cmd(struct unit *u, struct order *ord);
    int guard_off_cmd(struct unit *u, struct order *ord);
    int reshow_cmd(struct unit *u, struct order *ord);
    int mail_cmd(struct unit *u, struct order *ord);
    int claim_cmd(struct unit *u, struct order *ord);
    void transfer_faction(struct faction *fsrc, struct faction *fdst);
    void peasant_migration(struct region * r);

    void nmr_warnings(void);
    bool nmr_death(const struct faction * f, int turn, int timeout);

    bool cansee(const struct faction * f, const struct region * r,
        const struct unit *u, int modifier);
    bool cansee_unit(const struct unit *u, const struct region *r, const struct unit *who,
        int modifier);
    bool seefaction(const struct faction *f, const struct region *r,
        const struct unit *u, int modifier);
    int armedmen(const struct unit *u, bool siege_weapons);
    int peasant_luck_effect(int peasants, int luck, int maxp, double variance);

    #define FORCE_LEAVE_POSTCOMBAT 1
    #define FORCE_LEAVE_ALL 2
    bool rule_force_leave(int flag);
    bool LongHunger(const struct unit *u);
    int NMRTimeout(void);
    int NewbieImmunity(void);
    bool IsImmune(const struct faction *f);
    bool help_enter(struct unit *uo, struct unit *u);

    enum param_t findparam_ex(const char *s, const struct locale * lang);

#define QUIT_WITH_TRANSFER

#ifdef __cplusplus
}
#endif
#endif
