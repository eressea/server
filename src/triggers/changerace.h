#ifndef CHANGERACE_H
#define CHANGERACE_H
#ifdef __cplusplus
extern "C" {
#endif

    /* all types we use are defined here to reduce dependencies */
    struct trigger_type;
    struct trigger;
    struct unit;
    struct race;

    extern struct trigger_type tt_changerace;

    extern struct trigger *trigger_changerace(struct unit *u,
        const struct race *urace, const struct race *irace);

#ifdef __cplusplus
}
#endif
#endif
