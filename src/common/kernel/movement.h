/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_KRNL_MOVEMENT
#define H_KRNL_MOVEMENT
#ifdef __cplusplus
extern "C" {
#endif

struct unit;
struct ship;

/* ------------------------------------------------------------- */
/* die Zahlen sind genau äquivalent zu den race Flags */

#define MV_CANNOTMOVE     (1<<5)
#define MV_FLY            (1<<7)   /* kann fliegen */
#define MV_SWIM           (1<<8)   /* kann schwimmen */
#define MV_WALK           (1<<9)   /* kann über Land gehen */

/* ------------------------------------------------------------- */

/* Die tragekapaz. ist hardcodiert mit defines, da es bis jetzt sowieso nur 2
 * objecte gibt, die etwas tragen. */

#define SILVERWEIGHT       1

#define SCALEWEIGHT      100	/* Faktor, um den die Anzeige von gewichten
								 * * skaliert wird */

extern int personcapacity(const struct unit *u);

#define HORSECAPACITY   7000
#define WAGONCAPACITY  14000

#define HORSESNEEDED    2

/* ein mensch wiegt 10, traegt also 5, ein pferd wiegt 50, traegt also 20. ein
 * wagen wird von zwei pferden gezogen und traegt total 140, davon 40 die
 * pferde, macht nur noch 100, aber samt eigenem gewicht (40) macht also 140. */

extern direction_t getdirection(const struct locale *);
extern void movement(void);
extern int travel(struct unit * u, struct region * r2, int flucht, struct region_list** routep);
extern struct unit *is_guarded(struct region * r, struct unit * u, unsigned int mask);
extern int enoughsailors(const struct ship * sh, const struct region * r);
extern boolean canswim(struct unit *u);
extern struct unit *get_captain(const struct ship * sh);
extern void travelthru(const struct unit * u, struct region * r);
extern struct ship * move_ship(struct ship * sh, struct region * from, struct region * to, struct region_list * route);
extern int walkingcapacity(const struct unit * u);

extern void follow_unit(void);

struct building_type;
boolean buildingtype_exists(const struct region * r, const struct building_type * bt);
struct unit* owner_buildingtyp(const struct region * r, const struct building_type * bt);

extern struct attrib_type at_speedup;

#ifdef __cplusplus
}
#endif
#endif
