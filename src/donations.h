#pragma once

#ifndef H_DONATIONS
#define H_DONATIONS

#ifdef __cplusplus
extern "C" {
#endif

    struct faction;
    struct region;

    void add_donation(struct faction * f1, struct faction * f2, int amount, struct region * r);
    void free_donations(void);
    void report_donations(void);

#ifdef __cplusplus
}
#endif
#endif
