/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.

 */
#ifndef H_GC_GIVE
#define H_GC_GIVE
#ifdef __cplusplus
extern "C" {
#endif

    struct item_type;
    struct order;
    struct unit;
    struct message;

    int give_item(int want, const struct item_type *itype,
    struct unit *src, struct unit *dest, struct order *ord);
    struct message * disband_men(int n, struct unit * u, struct order *ord);
    struct message * give_men(int n, struct unit *u, struct unit *u2,
    struct order *ord);
    void give_unit(struct unit *u, struct unit *u2, struct order *ord);
    void give_cmd(struct unit * u, struct order * ord);
    struct message * check_give(const struct unit * u, const struct unit * u2, struct order *ord);
    bool can_give_to(struct unit *u, struct unit *u2);

#ifdef __cplusplus
}
#endif
#endif
