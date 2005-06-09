/* vi: set ts=2:
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

#ifndef H_KRNL_PLANES
#define H_KRNL_PLANES
#ifdef __cplusplus
extern "C" {
#endif

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
#define PFL_NOBUILD      1024  /* Bauen außer Betrieb */
#define PFL_NOFEED       2048  /* Kein Unterhalt nötig */
#define PFL_FRIENDLY     4096  /* everyone is your ally */
#define PFL_NOORCGROWTH  8192  /* orcs don't grow */
#define PFL_NOMONSTERS  16384  /* no monster randenc */
#define PFL_SEESPECIAL  32768  /* far seeing */

typedef struct watcher {
	struct watcher * next;
	struct faction * faction;
	unsigned char mode;
} watcher;

typedef struct plane {
	struct plane *next;
	struct watcher * watchers;
	int id;
	char *name;
	short minx, maxx, miny, maxy;
	unsigned int flags;
	struct attrib *attribs;
} plane;

extern struct plane *planes;

struct plane *getplane(const struct region *r);
struct plane *findplane(short x, short y);
void init_planes(void);
int getplaneid(const struct region *r);
struct plane * getplanebyid(int id);
short region_x(const struct region *r, const struct faction *f);
short region_y(const struct region *r, const struct faction *f);
short plane_center_x(const struct plane *pl);
short plane_center_y(const struct plane *pl);
void set_ursprung(struct faction *f, int id, short x, short y);
plane * create_new_plane(int id, const char *name, short minx, short maxx, short miny, short maxy, int flags);
plane * getplanebyname(const char *);
extern short rel_to_abs(const struct plane *pl, const struct faction * f, short rel, unsigned char index);
extern boolean is_watcher(const struct plane * p, const struct faction * f);
extern void * resolve_plane(variant data);
extern void write_plane_reference(const plane * p, FILE * F);
extern int read_plane_reference(plane ** pp, FILE * F);

#ifdef __cplusplus
}
#endif
#endif
