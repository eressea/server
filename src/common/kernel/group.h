/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_KERNEL_GROUP
#define H_KERNEL_GROUP
#ifdef __cplusplus
extern "C" {
#endif

/* bitfield value for group::flags */
#define GFL_ALIVE 0x01 /* There is at least one struct unit in the group */

typedef struct group {
	struct group * next;
	struct group * nexthash;
	struct faction * f;
	struct attrib *attribs;
	char * name;
	struct ally * allies;
	int flags;
	int gid;
	int members;
} group;

extern struct attrib_type at_group; /* attribute for units assigned to a group */
extern boolean join_group(struct unit * u, const char * name);
extern void set_group(struct unit * u, struct group * g);
extern void free_group(struct group * g);

extern void write_groups(FILE * F, struct group * g);
extern void read_groups(FILE * F, struct faction * f);

#ifdef __cplusplus
}
#endif
#endif
