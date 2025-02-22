#pragma once
#ifndef H_KRNL_CURSES
#define H_KRNL_CURSES

#include <stdbool.h>

void register_borders(void);

/* fuer Feuerwaende: in movement muss das noch explizit getestet werden.
    ** besser waere eine blcok_type::move() routine, die den effekt
    ** der Bewegung auf eine struct unit anwendet.
    **/
extern struct border_type bt_chaosgate;
extern struct border_type bt_firewall;

typedef struct wall_data {
    struct unit *mage;
    int force;
    bool active;
    int countdown;
} wall_data;

#endif
