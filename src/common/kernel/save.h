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

/* reading and writing the data files, reading the orders */

#ifndef H_KRNL_SAVE
#define H_KRNL_SAVE
#ifdef __cplusplus
extern "C" {
#endif

double version(void);

#define MAX_INPUT_SIZE	DISPLAYSIZE*2
/* Nach MAX_INPUT_SIZE brechen wir das Einlesen der Zeile ab und nehmen an,
 * dass hier ein Fehler (fehlende ") vorliegt */

FILE * cfopen(const char *filename, const char *mode);
int readorders(const char *);
int creategame(void);
int readgame(boolean backup);
void writegame(char *path, char quiet);

extern void rsf(FILE * F, char *s, size_t len);

extern boolean is_persistent(const char *s, const struct locale * lang);

/* Versionsänderungen: */
#define HEX_VERSION 81
extern int data_version;
extern int minfaction;
extern int maxregions;
extern int firstx, firsty;
extern int quiet;

extern void init_locales(void);
extern int init_data(const char * xmlfile);
extern int lastturn(void);

extern void read_items(FILE *f, struct item **it);
extern void write_items(FILE *f, struct item *it);
extern void a_read(FILE * f, struct attrib ** attribs);
extern void a_write(FILE * f, const struct attrib * attribs);

extern void write_faction_reference(const struct faction * f, FILE * F);
extern int read_faction_reference(struct faction ** f, FILE * F);

extern const char * datapath(void);

#if RESOURCE_CONVERSION
extern struct attrib_type at_resources;
#endif

extern void writeunit(FILE * stream, const struct unit * u);
extern struct unit * readunit(FILE * stream);

extern void writeregion(FILE * stream, const struct region * r);
extern struct region * readregion(FILE * stream, int x, int y);

extern void writefaction(FILE * stream, const struct faction * f);
extern struct faction * readfaction(FILE * stream);
extern char * getbuf(FILE * F);

#ifdef __cplusplus
}
#endif
#endif
