/* vi: set ts=2:
 *
 *	$Id: group.h,v 1.1 2001/01/25 09:37:57 enno Exp $
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

#ifndef GROUP_H
#define GROUP_H

/* bitfield value for group::flags */
#define GFL_ALIVE 0x01 /* There is at least one struct unit in the group */

typedef struct group {
	struct group * next;
	struct group * nexthash;
	struct faction * f;
	char * name;
	struct ally * allies;
	int flags;
	int gid;
	int members;
} group;

extern attrib_type at_group; /* attribute for units assigned to a group */
extern boolean join_group(struct unit * u, const char* name);
extern void free_group(group * g);

extern void write_groups(FILE * F, group * g);
extern void read_groups(FILE * F, struct faction * f);

#endif
