/* vi: set ts=2:
 *
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

#ifndef H_KRNL_POOL_H
#define H_KRNL_POOL_H
#ifdef __cplusplus
extern "C" {
#endif

#define res2item(res) ((item_t)((res)-R_MINITEM))
#define res2herb(res) ((herb_t)((res)-R_MINHERB))
#define res2potion(res) ((potion_t)((res)-R_MINPOTION))

#define item2res(itm) (resource_t)(itm+R_MINITEM)
#define herb2res(itm) (resource_t)(itm+R_MINHERB)
#define potion2res(itm) (resource_t)(itm+R_MINPOTION)

int get_pooled(const struct unit * u, const struct region * r, resource_t itm);
int use_pooled(struct unit * u, struct region * r, resource_t itm, int count);
int use_pooled_give(struct unit * u, struct region * r, resource_t itm, int count);

/** use_pooled
 * verbraucht 'count' Objekte der resource 'itm'
 * unter zuhilfenahme des Pools der struct region und Aufbrauch des
 * von der Einheit reservierten Resourcen
 *
 * use_pooled_give
 * verbraucht 'count' Objekte der resource 'itm' wie use_pooled, jedoch
 * zuerst vom nicht reservierten.
 * (wichtig bei gib, da sonst beim zweiten GIB des selben Gegenstandes
 * jede Reservierung gelöscht wird)
 */

/* bitfield values for get/use/change operations */
#define GET_SLACK      0x01
#define GET_RESERVE    0x02

#define GET_POOLED_SLACK    0x08
#define GET_POOLED_RESERVE  0x10
#define GET_POOLED_FORCE    0x20 /* ignore f->options pools */
#define GET_ALLIED_SLACK    0x30
#define GET_ALLIED_RESERVE  0x40

/* for convenience: */
#define GET_DEFAULT (GET_RESERVE|GET_SLACK|GET_POOLED_SLACK)


int get_all(const struct unit * u, resource_t itm);
int use_all(struct unit * u, resource_t itm, int count);

/** use_all
 * verbraucht 'count' Objekte der resource 'itm'
 * unter zuhilfenahme aller Einheiten der struct region
 */

int get_allied(const struct unit * u, struct region * r, resource_t itm);
int use_allied(struct unit * u, struct region * r, resource_t itm, int count);

/** use_allied
 * verbraucht 'count' Objekte der resource 'itm'
 * unter zuhilfenahme des alliierten in der struct region
 */

int get_reserved(const struct unit * u, resource_t itm);
int use_reserved(struct unit * u, resource_t itm, int count);

/** use_reserved
 * verbraucht 'count' Objekte der resource 'itm'
 * aus den reservierten Objekten der Einheit.
 */

int use_slack(struct unit * u, resource_t itm, int count);
int get_slack(const struct unit * u, resource_t itm);

/** use_slack
 * verbraucht 'count' Objekte der resource 'itm'
 * aus den nicht-reservierten Objekten der Einheit.
 */

int get_resource(const struct unit * u, resource_t res);
int change_resource(struct unit * u, resource_t res, int change);

int get_resvalue(const struct unit * u, resource_t resource);
int change_resvalue(struct unit * u, resource_t resource, int value);

int new_get_pooled(const struct unit * u, const struct resource_type * res, int mode);
int new_use_pooled(struct unit * u, const struct resource_type * res, int mode, int count);
	/** use_pooled
	 * verbraucht 'count' Objekte der resource 'itm'
	 * unter zuhilfenahme des Pools der struct region und Aufbrauch des
	 * von der Einheit reservierten Resourcen
	 */

int new_get_resource(const struct unit * u, const struct resource_type * res);
int new_change_resource(struct unit * u, const struct resource_type * res, int change);

int new_get_resvalue(const struct unit * u, const struct resource_type * res);
int new_change_resvalue(struct unit * u, const struct resource_type * res, int value);

void init_pool(void);

/** init_pool
 * initialisiert den regionalen Pool.
 */

#ifdef __cplusplus
}
#endif
#endif
