/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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

#ifndef REPORTS_H
#define REPORTS_H

/* Alter, ab dem der Score angezeigt werden soll: */
#define DISPLAYSCORE 12
/* Breite einer Reportzeile: */
#define REPORTWIDTH 78

extern const char *directions[];
extern const char *neue_gebiete[];

/* kann_finden speedups */
extern boolean kann_finden(struct faction * f1, struct faction * f2);
extern void add_find(struct faction *, struct unit *);
extern struct unit * can_find(struct faction *, struct faction *);
/* funktionen zum schreiben eines reports */
extern int read_datenames(const char *filename);
void sparagraph(struct strlist ** SP, const char *s, int indent, char mark);
void lparagraph(struct strlist ** SP, char *s, int indent, char mark);
const char *hp_status(const struct unit * u);
extern void spskill(const struct unit * u, skill_t sk, int *dh, int days); /* mapper */
extern void spunit(struct strlist ** SP, const struct faction * f, const struct unit * u, int indent, int mode);

void reports(void);
char *gamedate(void);
char *gamedate2(void);

struct summary;
extern void report_computer(FILE * F, struct faction * f);
extern void report_summary(struct summary * n, struct summary * o, boolean full);
extern struct summary * make_summary(boolean count_new);

int hat_in_region(item_t itm, struct region * r, struct faction * f);
int in_region(struct region * r, struct unit * u);

char *translate_regions(const char *st, struct faction * f);

char *replace_global_coords(const char *s, struct faction * f);

#ifdef USE_MERIAN
#ifdef FAST_REGION
void merian(FILE * out, struct faction *f);
#else
void merian(FILE * out, vset* regs, struct faction *f);
#endif
#endif
char *report_kampfstatus(const struct unit *u);

char *f_regionid(const struct region *r, const struct faction *f);

/* für fast_region und neuen CR: */

enum {
	see_none,
	see_neighbour,
	see_lighthouse,
	see_travel,
#ifdef SEE_FAR
	see_far,
#endif
	see_unit,
	see_battle
};

typedef struct seen_region {
	struct seen_region * next;
	struct seen_region * prev;
	struct seen_region * nextHash;
	struct region *r;
	unsigned char mode;
	boolean disbelieves;
} seen_region;

extern seen_region * seen;
extern const char* resname(resource_t res, int i);

extern char **seasonnames;
extern char **weeknames;
extern char **monthnames;
extern int  *season;
extern int  *storms;
extern char *agename;
extern int  seasons;
extern int  weeks_per_month;
extern int  months_per_year;

extern void report_item(const struct unit * owner, const struct item * i, const struct faction * viewer, const char ** name, const char ** basename, int * number, boolean singular);
extern void report_building(FILE *F, const struct region * r, const struct building * b, const struct faction * f, int mode);
extern int bufunit(const struct faction * f, const struct unit * u, int indent, int mode);

extern const char *neue_gebiete[];
extern const char *coasts[];
extern const char * reportpath(void);

#endif
