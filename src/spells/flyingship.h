#pragma once

#ifndef FLYINGSHIP_H
#define FLYINGSHIP_H

#include "magic.h"

#include <kernel/ship.h>

#ifdef __cplusplus
extern "C" {
#endif

    int sp_flying_ship(castorder * co);

    void register_flyingship(void);
    bool flying_ship(const ship * sh);
    int levitate_ship(struct ship *sh, struct unit *mage, double power,
        int duration);

#ifdef __cplusplus
}
#endif

#endif