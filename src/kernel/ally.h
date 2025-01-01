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
struct ally;

int ally_status(const char *s);
int ally_get(struct ally *a_allies, const struct faction *f);
struct ally *ally_set(struct ally **p_allies, struct faction *f, int status);
void write_allies(struct gamedata *data, const struct ally *a_allies);
void read_allies(struct gamedata * data, struct ally **sfp);
typedef int (*cb_allies_walk)(struct faction *, int, void *);
int allies_walk(struct ally *a_allies, cb_allies_walk callback, void *udata);
struct ally *allies_clone(const struct ally *al);

void allies_free(struct ally *al);

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
