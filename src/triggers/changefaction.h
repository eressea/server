#ifndef CHANGEFACTION_H
#define CHANGEFACTION_H
#ifdef __cplusplus
extern "C" {
#endif

    /* all types we use are defined here to reduce dependencies */
    struct trigger_type;
    struct trigger;

    struct unit;
    struct faction;

    extern struct trigger_type tt_changefaction;

    extern struct trigger *trigger_changefaction(struct unit *u,
    struct faction *f);

#ifdef __cplusplus
}
#endif
#endif
