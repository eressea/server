#ifndef H_TRG_SHOCK_H
#define H_TRG_SHOCK_H
#ifdef __cplusplus
extern "C" {
#endif

    struct trigger_type;
    struct trigger;

    struct unit;

    extern struct trigger_type tt_shock;
    extern struct trigger *trigger_shock(struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
