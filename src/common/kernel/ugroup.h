/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 $Id: ugroup.h,v 1.3 2001/04/29 19:27:42 enno Exp $
 */

#ifndef UGROUP_H
#define UGROUP_H

/* Globale Liste ugroups.
	Bei jeder unit u->ugroup zeigt auf aktuelle Gruppe.
	Gruppen mit members <= 1 werden vor den Reports
	aufgelöst. */

typedef struct ugroup {
	struct ugroup *next;
	int id;
	int members;
	struct unit **unit_array;
} ugroup;

boolean is_ugroupleader(const struct unit *u, const struct ugroup *ug);
ugroup *findugroup(const struct unit *u);
void ugroups(void);

#endif
