#pragma once

#ifndef H_GUARD
#define H_GUARD
#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct region;
    
    typedef enum { E_GUARD_OK, E_GUARD_UNARMED, E_GUARD_NEWBIE, E_GUARD_FLEEING } guard_t;

#define GUARD_NONE 0
#define GUARD_TAX 1
    /* Verhindert Steuereintreiben */
#define GUARD_MINING 2
    /* Verhindert Bergbau */
#define GUARD_TREES 4
    /* Verhindert Waldarbeiten */
#define GUARD_TRAVELTHRU 8
    /* Blockiert Durchreisende */
#define GUARD_LANDING 16
    /* Verhindert Ausstieg + Weiterreise */
#define GUARD_CREWS 32
    /* Verhindert Unterhaltung auf Schiffen */
#define GUARD_RECRUIT 64
    /* Verhindert Rekrutieren */
#define GUARD_PRODUCE 128
    /* Verhindert Abbau von Resourcen mit RTF_LIMITED */
#define GUARD_ALL 0xFFFF

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
