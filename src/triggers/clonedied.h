#ifndef CLONEDIED_H
#define CLONEDIED_H
#ifdef __cplusplus
extern "C" {
#endif

    struct trigger_type;
    struct trigger;

    struct unit;

    extern struct trigger_type tt_clonedied;
    struct trigger *trigger_clonedied(struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
