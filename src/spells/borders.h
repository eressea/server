#ifndef H_KRNL_CURSES
#define H_KRNL_CURSES
#ifdef __cplusplus
extern "C" {
#endif

    extern void register_borders(void);

    /* f�r Feuerw�nde: in movement mu� das noch explizit getestet werden.
     ** besser w�re eine blcok_type::move() routine, die den effekt
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

    extern const struct curse_type ct_firewall;

#ifdef __cplusplus
}
#endif
#endif
