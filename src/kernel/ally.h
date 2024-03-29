#ifndef ALLY_H
#define ALLY_H

#ifdef __cplusplus
extern "C" {
#endif

#define DONT_HELP      0
#define HELP_MONEY     1        /* Mitversorgen von Einheiten */
#define HELP_FIGHT     2        /* Bei Verteidigung mithelfen */
#define HELP_OBSERVE   4        /* Bei Wahrnehmung mithelfen */
#define HELP_GIVE      8        /* Dinge annehmen ohne KONTAKTIERE */
#define HELP_GUARD    16        /* Laesst Steuern eintreiben etc. */
#define HELP_FSTEALTH 32        /* Parteitarnung anzeigen. */
#define HELP_TRAVEL   64        /* Laesst Regionen betreten. */
#define HELP_ALL    (127-HELP_TRAVEL-HELP_OBSERVE)      /* Alle "positiven" HELPs zusammen */
/* HELP_OBSERVE deaktiviert */

extern struct attrib_type at_npcfaction;

struct faction;
struct group;
struct gamedata;
struct unit;
struct allies;

int ally_status(const char *s);
int ally_get(struct allies *al, const struct faction *f);
void ally_set(struct allies **p_al, struct faction *f, int status);
void write_allies(struct gamedata * data, const struct allies *alist);
void read_allies(struct gamedata * data, struct allies **sfp);
typedef int (*cb_allies_walk)(struct allies *, struct faction *, int, void *);
int allies_walk(struct allies *allies, cb_allies_walk callback, void *udata);
struct allies *allies_clone(const struct allies *al);

void allies_free(struct allies *al);

    int AllianceAuto(void);        /* flags that allied factions get automatically */
    int HelpMask(void);    /* flags restricted to allied factions */
    int alliedunit(const struct unit *u, const struct faction *f2,
        int mask);

    int alliedfaction(const struct faction *f, const struct faction *f2,
        int mask);
    int alliedgroup(const struct faction *f, const struct faction *f2,
        const struct group *g, int mask);
    int alliance_status(const struct faction *f, const struct faction *f2, int status);

#ifdef __cplusplus
}
#endif

#endif
