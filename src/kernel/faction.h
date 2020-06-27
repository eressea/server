#ifndef H_KRNL_FACTION
#define H_KRNL_FACTION

#include "skill.h"
#include "types.h"
#include "db/driver.h"

#include <util/resolve.h>
#include <modules/score.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct alliance;
    struct item;
    struct seen_region;
    struct attrib_type;
    struct gamedata;
    struct selist;

    /* faction flags */
#define FFL_NOAID         (1<<0)  /* Hilfsflag Kampf */
#define FFL_ISNEW         (1<<1)
#define FFL_PWMSG         (1<<2)  /* received a "new password" message */
#define FFL_QUIT          (1<<3)
#define FFL_CURSED        (1<<4)  /* you're going to have a bad time */
#define FFL_DEFENDER      (1<<10)
#define FFL_SELECT        (1<<22) /* ehemals f->dh, u->dh, r->dh, etc... */
#define FFL_MARK          (1<<23) /* fuer markierende algorithmen, die das 
                                   * hinterher auch wieder loeschen muessen!
                                   * (FFL_SELECT muss man vorher initialisieren,
                                   * FL_MARK hinterher loeschen) */
#define FFL_NOIDLEOUT     (1<<24) /* Partei stirbt nicht an NMRs */
#define FFL_NPC           (1<<25) /* eine Partei mit Monstern */
#define FFL_SAVEMASK (FFL_DEFENDER|FFL_NPC|FFL_NOIDLEOUT|FFL_CURSED)

    typedef struct origin {
        struct origin *next;
        int id;
        int x, y;
    } origin;

    typedef struct faction {
        struct faction *next;
        struct faction *nexthash;

        struct region *first;
        struct region *last;
        int no;
        int uid;
        int flags;
        char *name;
        dbrow_id banner_id;
        char *email;
        dbrow_id password_id;
        int max_spelllevel;
        struct spellbook *spellbook;
        const struct locale *locale;
        int lastorders;
        int age;
        struct origin *origin;
        const struct race *race;
        magic_t magiegebiet;
        int newbies;
        int num_people;             /* Anzahl Personen ohne Monster */
        int num_units;
        int options;
        struct allies *allies; /* alliedgroup and others should check sf.faction.alive before using a faction from f.allies */
        struct group *groups; /* alliedgroup and others should check sf.faction.alive before using a faction from f.groups */
        score_t score;
        struct alliance *alliance;
        int alliance_joindate;      /* the turn on which the faction joined its current alliance (or left the last one) */
        struct unit *units;
        struct attrib *attribs;
        struct message_list *msgs;
        struct bmsg {
            struct bmsg *next;
            struct region *r;
            struct message_list *msgs;
        } *battles;
        struct item *items;         /* items this faction can claim */
        struct selist *seen_factions;
        bool _alive;              /* enno: sollte ein flag werden */
    } faction;

    extern struct faction *factions;

#define WANT_OPTION(option) (1<<option)

    void fhash(struct faction *f);
    void funhash(struct faction *f);

    int faction_ally_status(const faction *f, const faction *f2);

    struct faction *findfaction(int n);
    int max_magicians(const faction * f);
    void set_show_item(faction * f, const struct item_type *itype);

    const char *factionname(const struct faction *f);
    struct unit *addplayer(struct region *r, faction * f);
    struct faction *addfaction(const char *email, const char *password,
        const struct race *frace, const struct locale *loc);
    bool checkpasswd(const faction * f, const char *passwd);
    int writepasswd(void);
    void destroyfaction(faction ** f);

    bool faction_alive(const struct faction *f);
    struct faction *faction_create(int no);

    struct alliance *f_get_alliance(const struct faction *f);

    void write_faction_reference(const struct faction *f,
        struct storage *store);
    int read_faction_reference(struct gamedata *data, struct faction **fp);

    void renumber_faction(faction * f, int no);
    void free_factions(void);
    void log_dead_factions(void);
    void remove_empty_factions(void);

    void update_interval(struct faction *f, struct region *r);

    const char *faction_getbanner(const struct faction *self);
    void faction_setbanner(struct faction *self, const char *name);

    const char *faction_getname(const struct faction *self);
    void faction_setname(struct faction *self, const char *name);

    const char *faction_getemail(const struct faction *self);
    void faction_setemail(struct faction *self, const char *email);

    char *faction_genpassword(struct faction *f, char *buffer);
    void faction_setpassword(struct faction *self, const char *pwhash);
    const char *faction_getpassword(const struct faction *f);
    bool valid_race(const struct faction *f, const struct race *rc);

    void faction_getorigin(const struct faction * f, int id, int *x, int *y);
    void faction_setorigin(struct faction * f, int id, int x, int y);

    struct spellbook * faction_get_spellbook(struct faction *f);

    /* skills */
    int faction_skill_limit(const struct faction *f, skill_t sk);
    int faction_count_skill(struct faction *f, skill_t sk);
    bool faction_id_is_unused(int);

#define COUNT_MONSTERS 0x01
#define COUNT_MIGRANTS 0x02
#define COUNT_DEFAULT  0x04

    int count_faction(const struct faction * f, int flags);
    int count_migrants(const struct faction * f);
    int count_maxmigrants(const struct faction * f);
    int max_magicians(const struct faction * f);

#define MONSTER_ID 666
    struct faction *getfaction(void);
    struct faction *get_monsters(void);
    struct faction *get_or_create_monsters(void);
    void save_special_items(struct unit *u);

#define is_monsters(f) (f->no==MONSTER_ID)

#ifdef __cplusplus
}
#endif
#endif
