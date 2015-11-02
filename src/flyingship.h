#pragma once

#ifndef FLYINGSHIP_H
#define FLYINGSHIP_H

#include "magic.h"

#include <kernel/ship.h>

#ifdef __cplusplus
extern "C" {
#endif

    void register_flyingship(void);

    int sp_flying_ship(castorder * co);

    struct curse *shipcurse_flyingship(struct ship *sh, struct unit *mage,
        double power, int duration);
    int levitate_ship(struct ship *sh, struct unit *mage, double power,
        int duration);
    bool flying_ship(const ship * sh);

#ifdef __cplusplus
}
#endif

#endif