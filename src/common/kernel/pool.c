/* vi: set ts=2:
 *
 *	$Id: pool.c,v 1.3 2001/02/09 13:53:51 corwin Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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

#include <config.h>
#include "eressea.h"
#include "pool.h"

#include "unit.h"
#include "faction.h"
#include "item.h"
#include "magic.h"
#include "region.h"
#include "race.h"

#include <assert.h>
#include <stdlib.h>

#define TODO_POOL
#undef TODO_RESOURCES

static int want_mp = 1 << O_MATERIALPOOL;
static int want_sp = 1 << O_SILBERPOOL;

#ifdef OLD_ITEMS
static resource_t
findresource(const char *name)
{
	item_t item;
	herb_t herb;
	potion_t potion;
	param_t param;

	item = finditem(name);
	if (item != NOITEM)
		return (resource_t) (R_MINITEM + item);

	herb = findherb(name);
	if (herb != NOHERB)
		return (resource_t) (R_MINHERB + herb);

	potion = findpotion(name);
	if (potion != NOPOTION)
		return (resource_t) (R_MINPOTION + potion);

	param = findparam(name);
	if (param == P_SILVER)
		return R_SILVER;

	return NORESOURCE;
}
#endif

#ifdef NEW_ITEMS
int
new_get_resource(const unit * u, const resource_type * rtype)
{
	const item_type * itype = resource2item(rtype);

	if (rtype->uget) {
		/* this resource is probably special */
		int i = rtype->uget(u, rtype);
		if (i>=0) return i;
	}
	if (itype!=NULL) {
		/* resouce is an item */
		if (u->race==RC_STONEGOLEM && itype == olditemtype[R_STONE]) {
			return u->number*GOLEM_STONE;
		} else if (u->race==RC_IRONGOLEM && itype == olditemtype[R_IRON]) {
			return u->number*GOLEM_IRON;
		} else {
			const item * i = *i_find((item**)&u->items, itype);
			if (i) return i->number;
			return 0;
		}
	}
	if (rtype == oldresourcetype[R_AURA])
		return get_spellpoints(u);
	if (rtype == oldresourcetype[R_PERMAURA])
		return max_spellpoints(u->region, u);
	if (rtype == oldresourcetype[R_HITPOINTS])
		return u->hp;
	if (rtype == oldresourcetype[R_PEASANTS])
		return rpeasants(u->region);
	if (rtype == oldresourcetype[R_PERSON])
		return u->number;
	assert(!"unbekannte ressource entdeckt");
	return 0;
}

int
new_change_resource(unit * u, const resource_type * rtype, int change)
{
	int i = 0;

	if (rtype->uchange)
		i = rtype->uchange(u, rtype, change);
	else if (rtype == oldresourcetype[R_AURA])
		i = change_spellpoints(u, change);
	else if (rtype == oldresourcetype[R_PERMAURA])
		i = change_maxspellpoints(u, change);
	else if (rtype == oldresourcetype[R_HITPOINTS])
		i = change_hitpoints(u, change);
	else if (rtype == oldresourcetype[R_PEASANTS]) {
		i = rpeasants(u->region) + change;
		if (i < 0) i = 0;
		rsetpeasants(u->region, i);
	}
	else
		assert(!"unbekannte resource entdeckt");
	assert(i >= 0 && (i < 1000000));	/* Softer Test, daß kein Unfug
										 * * passiert */
	return i;
}

int
new_get_resvalue(const unit * u, const resource_type * rtype)
{
	struct reservation * res = u->reservations;
	if (rtype==oldresourcetype[R_STONE] && u->race==RC_STONEGOLEM)
		return (u->number * GOLEM_STONE);
	if (rtype==oldresourcetype[R_IRON] && u->race==RC_IRONGOLEM)
		return (u->number * GOLEM_IRON);
	while (res && res->type!=rtype) res=res->next;
	if (res) return res->value;
	return 0;
}

int
new_change_resvalue(unit * u, const resource_type * rtype, int value)
{
	struct reservation *res, ** rp = &u->reservations;

	if (!value) return 0;

	while (*rp && (*rp)->type!=rtype) rp=&(*rp)->next;
	res = *rp;
	if (!res) {
		*rp = res = calloc(sizeof(struct reservation), 1);
		res->type = rtype;
		res->value = value;
	} else if (res && res->value+value<=0) {
		*rp = res->next;
		free(res);
		return 0;
	} else {
		res->value += value;
	}
	return res->value;
}

static int
new_set_resvalue(unit * u, const resource_type * rtype, int value)
{
	struct reservation *res, ** rp = &u->reservations;

	while (*rp && (*rp)->type!=rtype) rp=&(*rp)->next;
	res = *rp;
	if (!res) {
		if (!value) return 0;
		*rp = res = calloc(sizeof(struct reservation), 1);
		res->type = rtype;
		res->value = value;
	} else if (res && value<=0) {
		*rp = res->next;
		free(res);
		return 0;
	} else {
		res->value = value;
	}
	return res->value;
}

int
new_get_pooled(const unit * u, const resource_type * rtype, int mode)
{
	const faction * f = u->faction;
	unit *v;
	int use = 0;
	region * r = u->region;
	int have = new_get_resource(u, rtype);

	if ((mode & GET_SLACK) && (mode & GET_RESERVE)) use = have;
	else {
		int reserve = new_get_resvalue(u, rtype);
		int slack = max(0, have-reserve);
		if (mode & GET_RESERVE) use = have-slack;
		else if (mode & GET_SLACK) use = slack;
	}
	if (mode & ~(GET_SLACK|GET_RESERVE)) {
		for (v = r->units; v; v = v->next) if (u!=v) {
			int mask;

			if (u==v) continue;
			if (fval(v, FL_LOCKED)) continue;
			if (race[v->race].ec_flags & NOGIVE) continue;

			if (v->faction == f) {
				if ((mode & GET_POOLED_FORCE)==0) {
					if (rtype==r_silver && !(f->options & want_sp)) continue;
					if (rtype!=r_silver && !(f->options & want_mp)) continue;
				}
				mask = (mode >> 3) & (GET_SLACK|GET_RESERVE);
			}
			else if (allied(v, f, HELP_MONEY)) mask = (mode >> 6) & (GET_SLACK|GET_RESERVE);
			else continue;
			use += new_get_pooled(v, rtype, mask);
		}
	}
	return use;
}

int
new_use_pooled(unit * u, const resource_type * rtype, int mode, int count)
{
	const faction *f = u->faction;
	unit *v;
	int use = count;
	region * r = u->region;
	int n = 0, have = new_get_resource(u, rtype);

	if ((mode & GET_SLACK) && (mode & GET_RESERVE)) {
		n = min(use, have);
	} else {
		int reserve = new_get_resvalue(u, rtype);
		int slack = max(0, have-reserve);
		if (mode & GET_RESERVE) {
			n = have-slack;
			n = min(use, n);
			new_change_resvalue(u, rtype, -n);
		}
		else if (mode & GET_SLACK) {
			n = min(use, slack);
		}
	}
	if (n>0) {
		new_change_resource(u, rtype, -n);
		use -= n;
	}

	if (mode & ~(GET_SLACK|GET_RESERVE)) {
		for (v = r->units; v; v = v->next) if (u!=v) {
			int mask;
			if (v->faction == f) {
				if ((mode & GET_POOLED_FORCE)==0) {
					if (rtype==r_silver && !(f->options & want_sp)) continue;
					if (rtype!=r_silver && !(f->options & want_mp)) continue;
				}
				mask = (mode >> 3) & (GET_SLACK|GET_RESERVE);
			}
			else if (allied(v, f, HELP_MONEY)) mask = (mode >> 6) & (GET_SLACK|GET_RESERVE);
			else continue;
			use -= new_use_pooled(v, rtype, mask, use);
		}
	}
	return count-use;
}

#endif /* NEW_ITEMS */

int
get_resource(const unit * u, resource_t res)
{
#ifdef NEW_ITEMS
	return new_get_resource(u, oldresourcetype[res]);
#else /* NEW_ITEMS */
	if (res==R_STONE && u->race==RC_STONEGOLEM)
		return u->number*GOLEM_STONE;
	if (res==R_IRON && u->race==RC_IRONGOLEM)
		return u->number*GOLEM_IRON;

	if (is_item(res))
		return get_item(u, res2item(res));
	if (is_herb(res))
		return get_herb(u, res2herb(res));
	if (is_potion(res))
		return get_potion(u, res2potion(res));
	if (res == R_SILVER)
		return u->money;
	if (res == R_AURA)
		return get_spellpoints(u);
	if (res == R_PERMAURA)
		return max_spellpoints(u->region, u);
	if (res == R_HITPOINTS)
		return u->hp;
	if (res == R_PEASANTS)
		return rpeasants(u->region);
	/* TODO: Das ist natürlich Blödsinn. */
	if (res == R_UNIT)
		return 0;
	if (res == R_PERSON)
		return 0;
	assert(!"unbekannte ressource entdeckt");
	return 0;
#endif /* NEW_ITEMS */
}

int
change_resource(unit * u, resource_t res, int change)
{
	int i = 0;
#ifdef NEW_ITEMS
	const item_type * itype = resource2item(oldresourcetype[res]);
#endif

	if (res==R_STONE && u->race==RC_STONEGOLEM) {
	  i = u->number - (change+GOLEM_STONE-1)/GOLEM_STONE;
		scale_number(u, i);
	}
	else if (res==R_IRON && u->race==RC_IRONGOLEM) {
	  i = u->number - (change+GOLEM_IRON-1)/GOLEM_IRON;
		scale_number(u, i);
	}
#ifdef NEW_ITEMS
	else if (itype!=NULL) {
		item * it = i_change(&u->items, itype, change);
		if (it==NULL) return 0;
		return it->number;
	}
#else
	else if (is_item(res))
		i = change_item(u, res2item(res), change);
	else if (is_herb(res))
		i = change_herb(u, res2herb(res), change);
	else if (is_potion(res))
		i = change_potion(u, res2potion(res), change);
	else if (res == R_SILVER)
		i = change_money(u, change);
#endif
	else if (res == R_AURA)
		i = change_spellpoints(u, change);
	else if (res == R_PERMAURA)
		i = change_maxspellpoints(u, change);
	else if (res == R_HITPOINTS)
		i = change_hitpoints(u, change);
	else if (res == R_PEASANTS) {
		i = rpeasants(u->region) + change;
		if(i < 0) i = 0;
		rsetpeasants(u->region, i);
	}
	else
		assert(!"unbekannte ressource entdeckt");
	assert(i >= 0 && (i < 1000000));	/* Softer Test, daß kein Unfug
										 * * passiert */
	return i;
}

int
get_resvalue(const unit * u, resource_t resource)
{
#ifdef NEW_ITEMS
	const resource_type * rtype = oldresourcetype[resource];
	return new_get_resvalue(u, rtype);
#else
	if (resource==R_STONE && u->race==RC_STONEGOLEM)
		return (u->number * GOLEM_STONE);
	if (resource==R_IRON && u->race==RC_IRONGOLEM)
		return (u->number * GOLEM_IRON);

	if (resource == NORESOURCE)
		return 0;

	if (u->reserved) {
		return u->reserved[resource];
	}
	return 0;
#endif
}

static int
set_resvalue(unit * u, resource_t resource, int value)
{
#ifdef NEW_ITEMS
	return new_set_resvalue(u, oldresourcetype[resource], value);
#else
	if (!u->reserved)
		u->reserved = (int *) calloc(MAXRESOURCES, sizeof(int));
	return (u->reserved[resource] = value);
#endif
}

int
change_resvalue(unit * u, resource_t resource, int value)
{
	return set_resvalue(u, resource, get_resvalue(u, resource) + value);
}

int
get_reserved(const unit * u, resource_t resource)
{
#ifdef NEW_ITEMS
	return new_get_pooled(u, oldresourcetype[resource], GET_RESERVE);
#else
	int a = get_resvalue(u, resource);
	int b = get_resource(u, resource);
	return min(a, b);
#endif /* NEW_ITEMS */
}

int
use_reserved(unit * u, resource_t resource, int count)
{
#ifdef NEW_ITEMS
	return new_use_pooled(u, oldresourcetype[resource], GET_RESERVE, count);
#else
	int use = get_reserved(u, resource);

	use = min(use, count);
	change_resource(u, resource, -use);
	change_resvalue(u, resource, -use);

	return use;
#endif /* NEW_ITEMS */
}

int
get_slack(const unit * u, resource_t resource)
{
#ifdef NEW_ITEMS
	return new_get_pooled(u, oldresourcetype[resource], GET_SLACK);
#else /* NEW_ITEMS */
	int use = get_resource(u, resource);

	if (use <= 0)
		return 0;

	use -= get_resvalue(u, resource);
	if (use <= 0)
		return 0;

	return use;
#endif /* NEW_ITEMS */
}

int
use_slack(unit * u, resource_t resource, int count)
{
#ifdef NEW_ITEMS
	return new_use_pooled(u, oldresourcetype[resource], GET_SLACK, count);
#else /* NEW_ITEMS */
	int use = get_slack(u, resource);

	use = min(use, count);
	change_resource(u, resource, -use);
	return use;
#endif
}


int
get_pooled(const unit * u, const region * r, resource_t resource)
{
#ifdef NEW_ITEMS
	return new_get_pooled(u, oldresourcetype[resource], GET_DEFAULT);
#else /* NEW_ITEMS */
	const faction *f = u->faction;
	unit *v;
	int use = get_reserved(u, resource) + get_slack(u, resource);

	assert(u->region == r || r == NULL);
	if (r == NULL)
		r = u->region;
	assert(r);
	for (v = r->units; v; v = v->next)
		if (u != v && v->faction == f) {
			if (resource == R_SILVER && !(f->options & want_sp))
				continue;
			if (resource != R_SILVER && !(f->options & want_mp))
				continue;
			if (resource == R_SILVER || is_item(resource) || is_herb(resource) || is_potion(resource))
				use += get_slack(v, resource);
		}
	return use;
#endif /* NEW_ITEMS */
}


int
use_pooled(unit * u, region * r, resource_t resource, int count)
{
#ifdef NEW_ITEMS
	return new_use_pooled(u, oldresourcetype[resource], GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, count);
#else /* NEW_ITEMS */
	faction *f = u->faction;
	int use = count;

	assert(r == NULL || u->region == r);
	r = u->region;
	use -= use_reserved(u, resource, use);

	if (use) {
		use -= use_slack(u, resource, use);
		if (resource == R_SILVER && !(f->options & want_sp))
			return count - use;
		if (resource != R_SILVER && !(f->options & want_mp))
			return count - use;
		if (use && (resource == R_SILVER || is_item(resource) || is_herb(resource) || is_potion(resource))) {
			unit *v;

			if (r == NULL)
				r = findunitregion(u);
			for (v = r->units; v; v = v->next)
				if (v->faction == f
						&& !(fval(u,FL_LOCKED))
						&& !(race[u->race].ec_flags & NOGIVE))
				{
					use -= use_slack(v, resource, use);
					if (!use)
						break;
				}
		}
	}
	return count - use;
#endif
}

int
use_pooled_give(unit * u, region * r, resource_t resource, int count)
{
#ifdef NEW_ITEMS
	int use = count;
	use -= new_use_pooled(u, oldresourcetype[resource], GET_SLACK, use);
	if (use>0) use -= new_use_pooled(u, oldresourcetype[resource], GET_RESERVE|GET_POOLED_SLACK, use);
	return count-use;
#else /* NEW_ITEMS */
	faction *f = u->faction;
	int use = count;

	/* zuerst geben wir aus dem nichtreservierten Teil unserer Habe */
	use -= use_slack(u, resource, use);

	/* wenn das nicht reicht, versuchen wir erst aus unserem reservierten
	 * Gegenständen zu bedienen */
	if (use) {
		use -= use_reserved(u, resource, use);
	}

	assert(r == NULL || u->region == r);
	r = u->region;

	/* hat auch das nicht gereicht, versuchen wir es über den Pool */
	if (use) {
		if (resource == R_SILVER && !(f->options & want_sp))
			return count - use;
		if (resource != R_SILVER && !(f->options & want_mp))
			return count - use;
		if (use && (resource == R_SILVER || is_item(resource) || is_herb(resource) || is_potion(resource))) {
			unit *v;

			if (r == NULL)
				r = findunitregion(u);
			for (v = r->units; v; v = v->next)
				if (v->faction == f) {
					use -= use_slack(v, resource, use);
					if (!use)
						break;
				}
		}
	}
	return count - use;
#endif
}

int
get_all(const unit * u, resource_t resource)
{
#ifdef NEW_ITEMS
	return new_get_pooled(u, oldresourcetype[resource], GET_SLACK|GET_RESERVE|GET_POOLED_SLACK|GET_POOLED_RESERVE|GET_POOLED_FORCE);
#else
	region * r = u->region;
	faction * f = u->faction;
	unit *v;
	int use = get_resource(u, resource);

	for (v = r->units; v; v = v->next)
		if (u != v && v->faction == f) {
			if (resource == R_SILVER || is_item(resource) || is_herb(resource) || is_potion(resource))
				use += get_resource(v, resource);
		}
	return use;
#endif
}

int
use_all(unit * u, resource_t resource, int count)
{
#ifdef NEW_ITEMS
	return new_use_pooled(u, oldresourcetype[resource], GET_SLACK|GET_RESERVE|GET_POOLED_SLACK|GET_POOLED_RESERVE|GET_POOLED_FORCE, count);
#else
	region * r = u->region;
	faction * f = u->faction;
	int use = count;

	assert(r == NULL || u->region == r);
	use -= use_reserved(u, resource, use);

	if (use) {
		use -= use_slack(u, resource, use);
		if (use && (resource == R_SILVER || is_item(resource) || is_herb(resource) || is_potion(resource))) {
			unit *v;
			for (v = r->units; v; v = v->next)
				if (v->faction == f) {
					use -= use_slack(v, resource, use);
					if (!use)
						break;
				}
			if (use) for (v = r->units; v; v = v->next)
				if (v->faction == f) {
					use -= use_reserved(v, resource, use);
					if (!use)
						break;
				}
		}
	}
	return count - use;
#endif
}

void
init_pool(void)
{
	unit *u;
	region *r;

	/* Falls jemand diese Listen erweitert hat, muß er auch den R_* enum
	 * erweitert haben. */
	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			strlist *s;

			list_foreach(strlist, u->orders, s) {
				if (u->number > 0 && igetkeyword(s->s) == K_RESERVE
						&& (race[u->race].ec_flags & GETITEM)) {
					int count = geti();
					int use;
					char *what = getstrtoken();
					const resource_type * rtype = findresourcetype(what, u->faction->locale);
					if (rtype == NULL)
						list_continue(s);	/* nur mit resources implementiert */
					new_set_resvalue(u, rtype, 0);	/* make sure the pool is empty */
					use = new_use_pooled(u, rtype, GET_DEFAULT, count);
					if (use) {
						new_set_resvalue(u, rtype, use);
						new_change_resource(u, rtype, use);
					}
				}
			}
			list_next(s);
		}
	}
}

