#ifndef H_TRG_TIMEOUT_H
#define H_TRG_TIMEOUT_H
#ifdef __cplusplus
extern "C" {
#endif

    struct trigger_type;
    struct trigger;

    extern struct trigger_type tt_timeout;

    extern struct trigger *trigger_timeout(int time, struct trigger *callbacks);

#ifdef __cplusplus
}
#endif
#endif
