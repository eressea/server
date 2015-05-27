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

#ifndef H_KRNL_FACTION
#define H_KRNL_FACTION

#include "skill.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

    struct alliance;
    struct item;
    struct seen_region;

    /* SMART_INTERVALS: define to speed up finding the interval of regions that a
       faction is in. defining this speeds up the turn by 30-40% */
#define SMART_INTERVALS

    /* faction flags */
#define FFL_NEWID (1<<0)        /* Die Partei hat bereits einmal ihre no gewechselt */
#define FFL_ISNEW         (1<<1)
#define FFL_RESTART       (1<<2)
#define FFL_QUIT          (1<<3)
#define FFL_DEFENDER      (1<<10)
#define FFL_SELECT        (1<<18)       /* ehemals f->dh, u->dh, r->dh, etc... */
#define FFL_NOAID         (1<<21)       /* Hilfsflag Kampf */
#define FFL_MARK          (1<<23)       /* für markierende algorithmen, die das 
                                             * hinterher auch wieder löschen müssen!
                                             * (FFL_SELECT muss man vorher initialisieren,
                                             * FL_MARK hinterher löschen) */
#define FFL_NOIDLEOUT     (1<<24)       /* Partei stirbt nicht an NMRs */
#define FFL_NPC           (1<<25)       /* eine Partei mit Monstern */
#define FFL_DBENTRY       (1<<28)       /* Partei ist in Datenbank eingetragen */

#define FFL_SAVEMASK (FFL_DEFENDER|FFL_NEWID|FFL_NPC|FFL_DBENTRY|FFL_NOIDLEOUT)

    typedef struct faction {
        struct faction *next;
        struct faction *nexthash;

#ifdef SMART_INTERVALS
        struct region *first;
        struct region *last;
#endif
        int no;
        int subscription;
        int flags;
        char *name;
        char *banner;
        char *email;
        char *passw;
        int max_spelllevel;
        struct spellbook *spellbook;
        const struct locale *locale;
        int lastorders;
        int age;
        struct ursprung *ursprung;
        const struct race *race;
        magic_t magiegebiet;
        int newbies;
        int num_people;             /* Anzahl Personen ohne Monster */
        int num_total;              /* Anzahl Personen mit Monstern */
        int options;
        int no_units;
        struct ally *allies;
        struct group *groups;
        int nregions;
        int money;
#if SCORE_MODULE
        int score;
#endif
        struct alliance *alliance;
        int alliance_joindate;      /* the turn on which the faction joined its current alliance (or left the last one) */
#ifdef VICTORY_DELAY
        unsigned char victory_delay;
#endif
        struct unit *units;
        struct attrib *attribs;
        struct message_list *msgs;
        struct bmsg {
            struct bmsg *next;
            struct region *r;
            struct message_list *msgs;
        } *battles;
        struct item *items;         /* items this faction can claim */
        struct seen_region **seen;
        struct quicklist *seen_factions;
        bool alive;              /* enno: sollte ein flag werden */
    } faction;

    extern struct faction *factions;

    void fhash(struct faction *f);
    void funhash(struct faction *f);

    struct faction *findfaction(int n);
    int max_magicians(const faction * f);
    void set_show_item(faction * f, const struct item_type *itype);

    const struct unit *random_unit_in_faction(const struct faction *f);
    const char *factionname(const struct faction *f);
    struct unit *addplayer(struct region *r, faction * f);
    struct faction *addfaction(const char *email, const char *password,
        const struct race *frace, const struct locale *loc, int subscription);
    bool checkpasswd(const faction * f, const char *passwd);
    void destroyfaction(faction * f);

    void set_alliance(struct faction *a, struct faction *b, int status);
    int get_alliance(const struct faction *a, const struct faction *b);

    struct alliance *f_get_alliance(const struct faction *f);

    void write_faction_reference(const struct faction *f,
        struct storage *store);
    variant read_faction_reference(struct storage *store);
    int resolve_faction(variant data, void *addr);

    void renumber_faction(faction * f, int no);
    void free_faction(struct faction *f);
    void remove_empty_factions(void);

#ifdef SMART_INTERVALS
    void update_interval(struct faction *f, struct region *r);
#endif

    const char *faction_getbanner(const struct faction *self);
    void faction_setbanner(struct faction *self, const char *name);

    const char *faction_getname(const struct faction *self);
    void faction_setname(struct faction *self, const char *name);

    const char *faction_getemail(const struct faction *self);
    void faction_setemail(struct faction *self, const char *email);

    const char *faction_getpassword(const struct faction *self);
    void faction_setpassword(struct faction *self, const char *password);
    bool valid_race(const struct faction *f, const struct race *rc);

    void faction_getorigin(const struct faction * f, int id, int *x, int *y);
    void faction_setorigin(struct faction * f, int id, int x, int y);

    struct spellbook * faction_get_spellbook(struct faction *f);

    /* skills */
    int skill_limit(struct faction *f, skill_t sk);
    int count_skill(struct faction *f, skill_t sk);

#ifdef __cplusplus
}
#endif
#endif
