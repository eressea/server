#ifndef CREATECURSE_H
#define CREATECURSE_H
#ifdef __cplusplus
extern "C" {
#endif

    /* all types we use are defined here to reduce dependencies */
    struct curse_type;
    struct trigger_type;
    struct trigger;
    struct region;
    struct faction;
    struct unit;

    extern struct trigger_type tt_createcurse;

    struct trigger *trigger_createcurse(struct unit *mage,
    struct unit *target, const struct curse_type *ct, double vigour,
        int duration, double effect, int men);

#ifdef __cplusplus
}
#endif
#endif
