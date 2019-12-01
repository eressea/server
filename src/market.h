#ifndef H_GC_MARKET
#define H_GC_MARKET

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
    struct building;
    struct race;

    bool markets_module(void);
    void do_markets(void);

    int rc_luxury_trade(const struct race *rc);
    int rc_herb_trade(const struct race *rc);

#ifdef __cplusplus
}
#endif
#endif
