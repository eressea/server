#pragma once

#ifndef H_GUARD
#define H_GUARD

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct region;
    
    typedef enum { E_GUARD_OK, E_GUARD_UNARMED, E_GUARD_NEWBIE, E_GUARD_FLEEING, E_GUARD_TERRAIN } guard_t;

    guard_t can_start_guarding(const struct unit * u);
    void update_guards(void);
    void setguard(struct unit * u, bool enabled);
    void guard(struct unit *u);

    struct unit *is_guarded(struct region *r, struct unit *u);
    bool is_guard(const struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
