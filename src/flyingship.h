#pragma once

#ifndef FLYINGSHIP_H
#define FLYINGSHIP_H

#include "magic.h"

#ifdef __cplusplus
extern "C" {
#endif

    int levitate_ship(struct ship *sh, struct unit *mage, double power,
        int duration);

    int sp_flying_ship(castorder * co);

#ifdef __cplusplus
}
#endif

#endif