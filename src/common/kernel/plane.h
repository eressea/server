/* vi: set ts=2:
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

#ifndef PLANES_H
#define PLANES_H

#define PFL_NOCOORDS        1
#define PFL_NORECRUITS      2
#define PFL_NOALLIANCES     4
#define PFL_LOWSTEALING     8
#define PFL_NOGIVE         16  /* Übergaben sind unmöglich */
#define PFL_NOATTACK       32  /* Angriffe und Diebstähle sind unmöglich */
#define PFL_NOTERRAIN      64  /* Terraintyp wird nicht angezeigt TODO? */
#define PFL_NOMAGIC       128  /* Zaubern ist unmöglich */
#define PFL_NOSTEALTH     256  /* Tarnung außer Betrieb */
#define PFL_NOTEACH       512  /* Lehre außer Betrieb */
#define PFL_NOBUILD      1024  /* Lehre außer Betrieb */
#define PFL_NOFEED       2048  /* Kein Unterhalt nötig TODO */
#define PFL_FRIENDLY     4096  /* everyone is your ally */
#define PFL_NOORCGROWTH  8192  /* orcs don't grow */

#define PFL_MUSEUM PFL_NOCOORDS | PFL_NORECRUITS | PFL_NOGIVE | PFL_NOATTACK | PFL_NOTERRAIN | PFL_NOMAGIC | PFL_NOSTEALTH | PFL_NOTEACH | PFL_NOBUILD | PFL_NOFEED

typedef struct plane {
	struct plane *next;
	int id;
	char *name;
	int minx,maxx,miny,maxy;
	unsigned int flags;
	struct attrib *attribs;
} plane;

struct plane *planes;

struct plane *getplane(const struct region *r);
struct plane *findplane(int x, int y);
void	init_planes(void);
int    getplaneid(struct region *r);
struct plane * getplanebyid(int id);
int region_x(const struct region *r, const struct faction *f);
int region_y(const struct region *r, const struct faction *f);
int plane_center_x(plane *pl);
int plane_center_y(plane *pl);
int ursprung_x(const struct faction *f, const plane *pl);
int ursprung_y(const struct faction *f, const plane *pl);
void set_ursprung(struct faction *f, int id, int x, int y);
plane * create_new_plane(int id, const char *name, int minx, int maxx, int miny, int maxy, int flags);
plane * getplanebyname(const char *);
extern int rel_to_abs(const struct plane *pl, const struct faction * f, int rel, unsigned char index);

extern void * resolve_plane(void * data);
extern void write_plane_reference(const plane * p, FILE * F);
extern void read_plane_reference(plane ** pp, FILE * F);

#endif
