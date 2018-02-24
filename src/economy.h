/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef H_GC_ECONOMY
#define H_GC_ECONOMY

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* Welchen Teil des Silbers die Bauern fuer Unterhaltung ausgeben (1/20), und
     * wiviel Silber ein Unterhalter pro Talentpunkt bekommt. */

    /* Wieviele Silbermuenzen jeweils auf einmal "getaxed" werden. */

#define TAXFRACTION             10

    /* Wieviel Silber pro Talentpunkt geklaut wird. */

#define STEALINCOME             50

    /* Teil der Bauern, welche Luxusgueter kaufen und verkaufen (1/100). */

#define TRADE_FRACTION          100

    /* Wieviel Fremde eine Partei pro Woche aufnehmen kann */
#define MAXNEWBIES								5

    struct unit;
    struct region;
    struct faction;
    struct order;
    struct message;
    struct item_type;
    
    typedef struct econ_request {
        struct econ_request *next;
        struct unit *unit;
        int qty;
        union {
            struct {
                struct order *ord;
            } recruit;
            struct {
                int no;
                bool goblin;             /* stealing */
            } steal;
            struct {
                const struct luxury_type *ltype;    /* trading */
            } trade;
        } type;
    } econ_request;

    int income(const struct unit *u);
    int entertainmoney(const struct region *r);

    void economics(struct region *r);
    void produce(struct region *r);
    void auto_work(struct region *r);

    unsigned int expand_production(struct region * r, struct econ_request * requests, struct econ_request ***results);

    typedef enum income_t { IC_WORK, IC_ENTERTAIN, IC_TAX, IC_TRADE, IC_TRADETAX, IC_STEAL, IC_MAGIC, IC_LOOT } income_t;
    void add_income(struct unit * u, income_t type, int want, int qty);

    void maintain_buildings(struct region *r);
    void make_item(struct unit * u, const struct item_type * itype, int want);
    int make_cmd(struct unit *u, struct order *ord);
    void split_allocations(struct region *r);
    int give_control_cmd(struct unit *u, struct order *ord);
    void give_control(struct unit * u, struct unit * u2);

    void tax_cmd(struct unit * u, struct order *ord, struct econ_request ** taxorders);
    void expandtax(struct region * r, struct econ_request * taxorders);

    struct message * steal_message(const struct unit * u, struct order *ord);
    void steal_cmd(struct unit * u, struct order *ord, struct econ_request ** stealorders);
    void expandstealing(struct region * r, struct econ_request * stealorders);

    void add_recruits(struct unit * u, int number, int wanted);

#ifdef __cplusplus
}
#endif
#endif
