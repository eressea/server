#pragma once

#ifndef PIRACY_H
#define PIRACY_H

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct order;
    struct region;

    void piracy_cmd(struct unit * u, struct order *ord);
    void age_piracy(struct region *r);

#ifdef __cplusplus
}
#endif

#endif
