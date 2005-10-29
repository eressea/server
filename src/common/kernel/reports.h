/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#ifndef H_KRNL_REPORTS
#define H_KRNL_REPORTS
#ifdef __cplusplus
extern "C" {
#endif

/* Alter, ab dem der Score angezeigt werden soll: */
#define DISPLAYSCORE 12
/* Breite einer Reportzeile: */
#define REPORTWIDTH 78

extern const char *directions[];
extern const char *coasts[];

/* kann_finden speedups */
extern boolean kann_finden(struct faction * f1, struct faction * f2);
extern struct unit * can_find(struct faction *, struct faction *);

/* funktionen zum schreiben eines reports */
extern int read_datenames(const char *filename);
void sparagraph(struct strlist ** SP, const char *s, int indent, char mark);
void lparagraph(struct strlist ** SP, char *s, int indent, char mark);
const char *hp_status(const struct unit * u);
extern size_t spskill(char * pbuf, size_t siz, const struct locale * lang, const struct unit * u, skill_t sk, int *dh, int days); /* mapper */
extern void spunit(struct strlist ** SP, const struct faction * f, const struct unit * u, int indent, int mode);

extern int reports(void);
extern int write_reports(struct faction * f, time_t ltime);
extern int init_reports(void);

extern const struct unit *ucansee(const struct faction *f, const struct unit *u, const struct unit *x);

struct summary;
extern void report_summary(struct summary * n, struct summary * o, boolean full);
extern struct summary * make_summary(void);

int hat_in_region(item_t itm, struct region * r, struct faction * f);


char *f_regionid(const struct region *r, const struct faction *f);

/* für fast_region und neuen CR: */

enum {
	see_none,
	see_neighbour,
	see_lighthouse,
	see_travel,
	see_far,
	see_unit,
	see_battle
};

typedef struct seen_region {
  struct seen_region * nextHash;
  struct region *r;
  unsigned char mode;
  boolean disbelieves;
} seen_region;

extern struct seen_region * find_seen(struct seen_region * seehash[], const struct region * r);
extern boolean add_seen(struct seen_region * seehash[], struct region * r, unsigned char mode, boolean dis);
extern struct seen_region ** seen_init(void);
extern void seen_done(struct seen_region * seehash[]);
extern void free_seen(void);
extern void get_seen_interval(struct seen_region ** seen, struct region ** first, struct region ** last);
  
extern void report_item(const struct unit * owner, const struct item * i, const struct faction * viewer, const char ** name, const char ** basename, int * number, boolean singular);
extern void report_building(FILE *F, const struct region * r, const struct building * b, const struct faction * f, int mode);
extern int bufunit(const struct faction * f, const struct unit * u, int indent, int mode);
#ifdef USE_UGROUPS
extern int bufunit_ugroupleader(const struct faction * f, const struct unit * u, int indent, int mode);
#endif

extern const char * reportpath(void);
extern struct faction_list * get_addresses(struct faction * f, struct seen_region * seehash[]);
extern const char * trailinto(const struct region * r, const struct locale * lang);

#ifdef __cplusplus
}
#endif
#endif
