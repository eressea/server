#pragma once

struct trigger_type;
struct trigger;

typedef struct timeout_data {
    struct trigger *triggers;
    int timer;
} timeout_data;

extern struct trigger_type tt_timeout;

struct trigger *trigger_timeout(int time, struct trigger *callbacks);
struct trigger* get_timeout(struct attrib* alist, const char* event, struct trigger_type* action);
