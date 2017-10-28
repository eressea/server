#ifndef H_KRNL_CURSES
#define H_KRNL_CURSES

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    extern void register_borders(void);

    /* für Feuerwände: in movement muß das noch explizit getestet werden.
     ** besser wäre eine blcok_type::move() routine, die den effekt
     ** der Bewegung auf eine struct unit anwendet.
     **/
    extern struct border_type bt_chaosgate;
    extern struct border_type bt_firewall;
    extern const struct curse_type ct_firewall;

    typedef struct wall_data {
        struct unit *mage;
        int force;
        bool active;
        int countdown;
    } wall_data;


#ifdef __cplusplus
}
#endif
#endif
