#pragma once

#ifndef H_GUARD
#define H_GUARD
#ifdef __cplusplus
extern "C" {
#endif

    struct unit;

    typedef enum { E_GUARD_OK, E_GUARD_UNARMED, E_GUARD_NEWBIE, E_GUARD_FLEEING } guard_t;

    extern struct attrib_type at_guard;

    guard_t can_start_guarding(const struct unit * u);
    void update_guards(void);
    unsigned int guard_flags(const struct unit * u);
    unsigned int getguard(const struct unit * u);
    void setguard(struct unit * u, unsigned int flags);

    void guard(struct unit * u, unsigned int mask);

    struct unit *is_guarded(struct region *r, struct unit *u, unsigned int mask);
    bool is_guard(const struct unit *u, unsigned int mask);

#ifdef __cplusplus
}
#endif
#endif
