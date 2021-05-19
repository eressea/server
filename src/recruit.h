#pragma once
#ifndef H_GC_RECRUIT
#define H_GC_RECRUIT

#ifdef __cplusplus
extern "C" {
#endif

    struct message;
    struct order;
    struct race;
    struct region;
    struct unit;

    struct message *can_recruit(struct unit *u, const struct race *rc, struct order *ord, int now);
    int max_recruits(const struct region *r);
    void add_recruits(struct unit* u, int number, int wanted, int ordered);
    void recruit(struct region * r);

#ifdef __cplusplus
}
#endif
#endif
