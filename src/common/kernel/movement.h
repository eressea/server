/* vi: set ts=2:
 *
 *	$Id: movement.h,v 1.2 2001/01/26 16:19:40 enno Exp $
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

#ifndef MOVE_H
#define MOVE_H

struct unit;
struct ship;

/* ------------------------------------------------------------- */

/* Die tragekapaz. ist hardcodiert mit defines, da es bis jetzt sowieso nur 2
 * objecte gibt, die etwas tragen. */

#define SILVERWEIGHT       1

#define SCALEWEIGHT      100	/* Faktor, um den die Anzeige von gewichten
								 * * skaliert wird */

#define INC_CAPACITIES 0
#if INC_CAPACITIES
#define PERSONCAPACITY(u)  (race[(u)->race].weight+600)
#define HORSECAPACITY   7700
#define WAGONCAPACITY  15400
#else
#define PERSONCAPACITY(u)  (race[(u)->race].weight+540)
#define HORSECAPACITY   7000
#define WAGONCAPACITY  14000
#endif

#define HORSESNEEDED    2

/* ein mensch wiegt 10, traegt also 5, ein pferd wiegt 50, traegt also 20. ein
 * wagen wird von zwei pferden gezogen und traegt total 140, davon 40 die
 * pferde, macht nur noch 100, aber samt eigenem gewicht (40) macht also 140. */

extern direction_t getdirection(void);
extern void movement(void);

extern direction_t * travel(struct region * r, struct unit * u, struct region * r2, int flucht);
extern struct unit *is_guarded(struct region * r, struct unit * u, unsigned int mask);
extern int enoughsailors(struct region * r, struct ship * sh);
extern boolean cansail(const struct region * r, struct ship * sh);
extern boolean canswim(struct unit *u);
extern struct unit *kapitaen(struct region * r, struct ship * sh);
extern void travelthru(struct unit * u, struct region * r);
extern ship * move_ship(ship * sh, struct region * from, struct region * to, struct region ** route);

extern attrib_type at_piracy_direction;

void follow(void);

struct building_type;
struct unit *gebaeude_vorhanden(const struct region * r, const struct building_type * bt);
#endif
