/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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
int readorders(const char *filename);
int creategame(void);
extern int readgame(const char * filename, int mode, int backup);
int writegame(const char *filename, int mode);

extern void rsf(FILE * F, char *s, size_t len);

/* Versionsänderungen: */
extern int data_version;
extern const char *game_name;
extern int enc_gamedata;

extern void init_locales(void);
extern int current_turn(void);

extern void read_items(struct storage * store, struct item **it);
extern void write_items(struct storage * store, struct item *it);

extern void write_unit(struct storage * store, const struct unit * u);
extern struct unit * read_unit(struct storage * store);

extern int a_readint(struct attrib * a, void * owner, struct storage * store);
extern void a_writeint(const struct attrib * a, const void * owner, struct storage * store);
extern int a_readshorts(struct attrib * a, void * owner, struct storage * store);
extern void a_writeshorts(const struct attrib * a, const void * owner, struct storage * store);
extern int a_readchars(struct attrib * a, void * owner, struct storage * store);
extern void a_writechars(const struct attrib * a, const void * owner, struct storage * store);
extern int a_readvoid(struct attrib * a, void * owner, struct storage * store);
extern void a_writevoid(const struct attrib * a, const void * owner, struct storage * store);
extern int a_readstring(struct attrib * a, void * owner, struct storage * store);
extern void a_writestring(const struct attrib * a, const void * owner, struct storage * store);
extern void a_finalizestring(struct attrib * a);

extern int freadstr(FILE * F, int encoding, char * str, size_t size);
extern int fwritestr(FILE * F, const char * str);

extern void create_backup(char *file);

#ifdef __cplusplus
}
#endif
#endif
