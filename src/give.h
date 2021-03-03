#ifndef H_GC_GIVE
#define H_GC_GIVE

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct item_type;
    struct order;
    struct unit;
    struct message;
    enum param_t;

    int give_item(int want, const struct item_type *itype,
    struct unit *src, struct unit *dest, struct order *ord);
    struct message * disband_men(int n, struct unit * u, struct order *ord);
    struct message * give_men(int n, struct unit *u, struct unit *u2,
    struct order *ord);
    int give_unit_allowed(const struct unit * u);
    void give_unit(struct unit *u, struct unit *u2, struct order *ord);
    void give_unit_cmd(struct unit* u, struct order* ord);
    enum param_t give_cmd(struct unit * u, struct order * ord);
    struct message * check_give(const struct unit * u, const struct unit * u2, struct order *ord);
    bool can_give_to(struct unit *u, struct unit *u2);
    bool rule_transfermen(void);

#ifdef __cplusplus
}
#endif
#endif
