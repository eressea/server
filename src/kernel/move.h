/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
                         Katja Zedel <katze@felidae.kn-bremen.de
                         Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef H_KRNL_MOVEMENT
#define H_KRNL_MOVEMENT
#ifdef __cplusplus
extern "C" {
#endif

struct unit;
struct ship;
struct building_type;

/* die Zahlen sind genau äquivalent zu den race Flags */
#define MV_CANNOTMOVE     (1<<5)
#define MV_FLY            (1<<7)   /* kann fliegen */
#define MV_SWIM           (1<<8)   /* kann schwimmen */
#define MV_WALK           (1<<9)   /* kann über Land gehen */

/* Die tragekapaz. ist hardcodiert mit defines, da es bis jetzt sowieso nur 2
** objekte gibt, die etwas tragen. */
#define SILVERWEIGHT       1
#define SCALEWEIGHT      100	/* Faktor, um den die Anzeige von gewichten
								 * * skaliert wird */
#define HORSECAPACITY   7000
#define WAGONCAPACITY  14000

#define HORSESNEEDED    2

/* ein mensch wiegt 10, traegt also 5, ein pferd wiegt 50, traegt also 20. ein
** wagen wird von zwei pferden gezogen und traegt total 140, davon 40 die
** pferde, macht nur noch 100, aber samt eigenem gewicht (40) macht also 140. */

int personcapacity(const struct unit *u);
direction_t getdirection(const struct locale *);
void movement(void);
void run_to(struct unit * u, struct region * to);
struct unit *is_guarded(struct region * r, struct unit * u, unsigned int mask);
boolean is_guard(const struct unit * u, int mask);
int enoughsailors(const struct ship * sh, const struct region * r);
boolean canswim(struct unit *u);
boolean canfly(struct unit *u);
struct unit *get_captain(const struct ship * sh);
void travelthru(const struct unit * u, struct region * r);
struct ship * move_ship(struct ship * sh, struct region * from, struct region * to, struct region_list * route);
int walkingcapacity(const struct unit * u);
void follow_unit(struct unit * u);
boolean buildingtype_exists(const struct region * r, const struct building_type * bt, boolean working);
struct unit* owner_buildingtyp(const struct region * r, const struct building_type * bt);

extern struct attrib_type at_speedup;

#ifdef __cplusplus
}
#endif
#endif
