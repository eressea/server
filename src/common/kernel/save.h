/* vi: set ts=2:
 *
 *	$Id: save.h,v 1.1 2001/01/25 09:37:57 enno Exp $
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

/* reading and writing the data files, reading the orders */

#ifndef SAVE_H
#define SAVE_H

double version(void);

#define MAX_INPUT_SIZE	DISPLAYSIZE*2
/* Nach MAX_INPUT_SIZE brechen wir das Einlesen der Zeile ab und nehmen an,
 * dass hier ein Fehler (fehlende ") vorliegt */

extern int inside_only;

FILE * cfopen(const char *filename, const char *mode);
int readorders(const char *);
int creategame(void);
void initgame(void);
int readgame(boolean backup);
void writeadresses(const char * name);
void writegame(char *path, char quiet);

extern void rsf(FILE * F, char *s, size_t len);

extern boolean is_persistent(const char *s);

/* Versionsänderungen: */
#define HEX_VERSION 81
extern int data_version;
extern int minfaction;
extern int maxregions;
extern int firstx, firsty;
extern int quiet;

extern void init_locales(void);
extern int lastturn(void);

extern void read_items(FILE *f, struct item **it);
extern void write_items(FILE *f, struct item *it);
extern void a_read(FILE * f, struct attrib ** attribs);
extern void a_write(FILE * f, const struct attrib * attribs);

extern void write_faction_reference(const struct faction * f, FILE * F);
extern int read_faction_reference(struct faction ** f, FILE * F);

extern const char * datapath(void);

#endif
