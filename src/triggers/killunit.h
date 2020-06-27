#ifndef KILLUNIT_H
#define KILLUNIT_H
#ifdef __cplusplus
extern "C" {
#endif

    struct trigger_type;
    struct trigger;

    struct unit;

    extern struct trigger_type tt_killunit;
    extern struct trigger *trigger_killunit(struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
