/*
+-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
| (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
|                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
+-------------------+  Stefan Reich <reich@halbling.de>

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.

*/
#ifndef H_GC_MARKET
#define H_GC_MARKET

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
    struct building;

    bool markets_module(void);
    void do_markets(void);

#ifdef __cplusplus
}
#endif
#endif
