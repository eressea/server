#pragma once

#ifndef FLYINGSHIP_H
#define FLYINGSHIP_H

#ifdef __cplusplus
extern "C" {
#endif

    int sp_flying_ship(struct castorder * co);

    void register_flyingship(void);
    bool flying_ship(const struct ship * sh);
    int levitate_ship(struct ship *sh, struct unit *mage, double power,
        int duration);

#ifdef __cplusplus
}
#endif

#endif