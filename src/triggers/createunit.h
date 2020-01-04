#ifndef CREATEUNIT_H
#define CREATEUNIT_H
#ifdef __cplusplus
extern "C" {
#endif

    /* all types we use are defined here to reduce dependencies */
    struct trigger_type;
    struct trigger;
    struct region;
    struct faction;
    struct unit;
    struct race;

    extern struct trigger_type tt_createunit;

    struct trigger *trigger_createunit(struct region *r, struct faction *f,
        const struct race *rc, int number);

#ifdef __cplusplus
}
#endif
#endif
