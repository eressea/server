#ifndef GATE_H
#define GATE_H
#ifdef __cplusplus
extern "C" {
#endif

    /* all types we use are defined here to reduce dependencies */
    struct trigger_type;
    struct trigger;
    struct region;
    struct building;

    extern struct trigger_type tt_gate;

#ifdef __cplusplus
}
#endif
#endif
