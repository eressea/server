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
#define GET_ALL (GET_SLACK|GET_RESERVE|GET_POOLED_SLACK|GET_POOLED_RESERVE|GET_POOLED_FORCE)

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
