#pragma once

/* all types we use are defined here to reduce dependencies */
struct trigger_type;
struct trigger;
struct unit;
struct race;

typedef struct changerace_data {
    struct unit *u;
    const struct race *race;
    const struct race *irace;
} changerace_data;

extern struct trigger_type tt_changerace;

struct trigger *change_race(struct unit *u, int duration, const struct race *urace, const struct race *irace);
void restore_race(struct unit *u, const struct race *rc);
struct trigger *get_change_race_trigger(struct unit *u);
