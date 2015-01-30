/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

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

    struct trigger *trigger_gate(struct building *b, struct region *r);

#ifdef __cplusplus
}
#endif
#endif
