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
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef BORDER_H
#define BORDER_H

extern unsigned int nextborder;

typedef struct border {
	struct border_type * type; /* the type of this border */
	struct border * next; /* next border between these regions */
	struct border * nexthash; /* for internal use only */
	struct region * from, * to; /* borders can be directed edges */
	attrib * attribs;
	void * data;
	unsigned int id; /* unique id */
} border;

typedef struct border_type {
    const char* __name; /* internal use only */
	boolean (*transparent)(const border *, const struct faction *);
		/* is it possible to see through this? */
	void (*init)(border *);
		/* constructor: initialize the border. allocate extra memory if needed */
	void (*destroy)(border *);
		/* destructor: remove all extra memory for destruction */
	void (*read)(border *, FILE *);
	void (*write)(const border *, FILE *);
	boolean (*block)(const border *, const struct unit *, const struct region * r);
		/* return true if it blocks movement of u from
		 * r to the opposite struct region.
		 * warning: struct unit may be NULL.
		 */
	const char *(*name)(const border * b, const struct region * r, const struct faction * f, int gflags);
		/* for example "a wall of fog" or "a doorway from r1 to r2"
		 * may depend on the struct faction, for example "a wall" may
		 * turn out to be "an illusionary wall"
		 */
	boolean (*rvisible)(const border *, const struct region *);
		/* is it visible to everyone in r ?
		 * if not, it may still be fvisible() for some f.
		 */
	boolean (*fvisible)(const border *, const struct faction *, const struct region *);
		/* is it visible to units of f in r?
		 * the function shall not check for
		 * existence of a struct unit in r. Example: a spell
		 * may be visible only to the struct faction that created it and thus
		 * appear in it's report (if there is a struct unit of f in r, which
		 * the reporting function will have to assure).
		 * if not true, it may still be uvisible() for some u.
		 */
	boolean (*uvisible)(const border *, const struct unit *);
		/* is it visible to u ?
		 * a doorway may only be visible to a struct unit with perception > 5
		 */
	boolean (*valid)(const border *);
		/* is the border in a valid state,
		 * or should it be erased at the end of this turn to save space?
		 */
	void (*move)(const border *, struct unit * u, const struct region * from, const struct region * to);
		/* executed when the units traverses this border */
	struct border_type * next; /* for internal use only */
} border_type;


extern border * find_border(unsigned int id);
void * resolve_borderid(void * data);

extern border * get_borders(const struct region * r1, const struct region * r2);
	/* returns the list of borders between r1 and r2 or r2 and r1 */
extern border * new_border(border_type * type, struct region * from, struct region * to);
	/* creates a border of the specified type */
extern void erase_border(border * b);
	/* remove the border from memory */
extern border_type * find_bordertype(const char * name);
extern void register_bordertype(border_type * type);
	/* register a new bordertype */

extern void read_borders(FILE * f);
extern void write_borders(FILE * f);
extern void age_borders(void);

/* provide default implementations for some member functions: */
extern void b_read(border * b, FILE *f);
extern void b_write(const border * b, FILE *f);
extern boolean b_blockall(const border *, const struct unit *, const struct region *);
extern boolean b_blocknone(const border *, const struct unit *, const struct region *);
extern boolean b_rvisible(const border *, const struct region * r);
extern boolean b_fvisible(const border *, const struct faction * f, const struct region *);
extern boolean b_uvisible(const border *, const struct unit * u);
extern boolean b_rinvisible(const border *, const struct region * r);
extern boolean b_finvisible(const border *, const struct faction * f, const struct region *);
extern boolean b_uinvisible(const border *, const struct unit * u);
extern boolean b_transparent(const border *, const struct faction *);
extern boolean b_opaque(const border *, const struct faction *); /* !transparent */

extern border_type bt_fogwall;
extern border_type bt_noway;
extern border_type bt_wall;
extern border_type bt_illusionwall;
extern border_type bt_road;

extern attrib_type at_countdown;

#endif
