#ifndef H_GC_ECONOMY
#define H_GC_ECONOMY

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* Welchen Teil des Silbers die Bauern fuer Unterhaltung ausgeben (1/20), und
     * wiviel Silber ein Unterhalter pro Talentpunkt bekommt. */

    /* Wieviele Silbermuenzen jeweils auf einmal "getaxed" werden. */
#define TAXFRACTION       10
#define ENTERTAINFRACTION 20

    /* Wieviel Silber pro Talentpunkt geklaut wird. */

#define STEALINCOME             50

    /* Teil der Bauern, welche Luxusgueter kaufen und verkaufen (1/100). */

#define TRADE_FRACTION          100

    /* Wieviel Fremde eine Partei pro Woche aufnehmen kann */
#define MAXNEWBIES								5

    struct unit;
    struct race;
    struct region;
    struct faction;
    struct order;
    struct message;
    struct terrain_type;
    struct item_type;
    
    typedef struct econ_request {
        struct unit *unit;
        int qty;
        enum econ_type {
            ECON_LIST,
            ECON_ENTERTAIN,
            ECON_WORK,
            ECON_TAX,
            ECON_LOOT,
            ECON_BUY,
            ECON_SELL,
            ECON_STEAL,
        } type;
        union {
            struct {
                int no;
                bool goblin;             /* stealing */
            } steal;
            struct {
                const struct luxury_type *ltype;    /* trading */
            } trade;
        } data;
    } econ_request;

    int expand_production(struct region * r, struct econ_request * requests, struct econ_request ***results);

    int income(const struct unit *u);
    int entertainmoney(const struct region *r);

    void do_give(struct region *r);
    void destroy(struct region *r);
    void produce(struct region *r);
    void auto_work(struct region *r);
    int max_luxuries_sold(const struct region* r);
    
    bool trade_needs_castle(const struct terrain_type *terrain, const struct race *rc);

    typedef enum income_t { IC_WORK, IC_ENTERTAIN, IC_TAX, IC_TRADE, IC_TRADETAX, IC_STEAL, IC_MAGIC, IC_LOOT } income_t;
    void add_income(struct unit * u, income_t type, int want, int qty);

    void maintain_buildings(struct region *r);
    void make_item(struct unit * u, const struct item_type * itype, int want);
    int make_cmd(struct unit *u, struct order *ord);
    int forget_cmd(struct unit *u, struct order *ord);
    void split_allocations(struct region *r);
    int give_control_cmd(struct unit *u, struct order *ord);
    void give_control(struct unit * u, struct unit * u2);

    void tax_cmd(struct unit * u, struct order *ord, struct econ_request ** taxorders);
    void expandtax(struct region * r, struct econ_request * taxorders);

    struct message * steal_message(const struct unit * u, struct order *ord);
    void steal_cmd(struct unit * u, struct order *ord, struct econ_request ** stealorders);
    int destroy_cmd(struct unit* u, struct order* ord);
    void expandstealing(struct region * r, struct econ_request * stealorders);

#ifdef __cplusplus
}
#endif
#endif
