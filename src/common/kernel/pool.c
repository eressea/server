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

#include "faction.h"
#include "item.h"
#include "magic.h"
#include "order.h"
#include "region.h"
#include "race.h"
#include "unit.h"

#include <assert.h>
#include <stdlib.h>

#define TODO_POOL
#undef TODO_RESOURCES

static int want_mp = 1 << O_MATERIALPOOL;
static int want_sp = 1 << O_SILBERPOOL;

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
		race_t urc = old_race(u->race);
		/* resouce is an item */
		if (urc==RC_STONEGOLEM && itype == olditemtype[R_STONE]) {
			return u->number*GOLEM_STONE;
		} else if (urc==RC_IRONGOLEM && itype == olditemtype[R_IRON]) {
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
    assert(!"undefined resource detected. rtype->uchange not initialized.");
  assert(i >= 0 && (i < 100000000));	/* Softer Test, daß kein Unfug
                                        * * passiert */
  return i;
}

int
new_get_resvalue(const unit * u, const resource_type * rtype)
{
	race_t urc = old_race(u->race);
	struct reservation * res = u->reservations;
	if (rtype==oldresourcetype[R_STONE] && urc==RC_STONEGOLEM)
		return (u->number * GOLEM_STONE);
	if (rtype==oldresourcetype[R_IRON] && urc==RC_IRONGOLEM)
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
	if (rtype->flags & RTF_POOLED && mode & ~(GET_SLACK|GET_RESERVE)) {
		for (v = r->units; v; v = v->next) if (u!=v) {
			int mask;

			if (u==v) continue;
			if (fval(v, UFL_LOCKED)) continue;
			if (urace(v)->ec_flags & NOGIVE) continue;

			if (v->faction == f) {
				if ((mode & GET_POOLED_FORCE)==0) {
					if (rtype==r_silver && !(f->options & want_sp)) continue;
					if (rtype!=r_silver && !(f->options & want_mp)) continue;
				}
				mask = (mode >> 3) & (GET_SLACK|GET_RESERVE);
			}
			else if (alliedunit(v, f, HELP_MONEY)) mask = (mode >> 6) & (GET_SLACK|GET_RESERVE);
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

	if (rtype->flags & RTF_POOLED && mode & ~(GET_SLACK|GET_RESERVE)) {
		for (v = r->units; v; v = v->next) if (u!=v) {
			int mask;
			if (urace(v)->ec_flags & NOGIVE) continue;
			if (v->faction == f) {
				if ((mode & GET_POOLED_FORCE)==0) {
					if (rtype==r_silver && !(f->options & want_sp)) continue;
					if (rtype!=r_silver && !(f->options & want_mp)) continue;
				}
				mask = (mode >> 3) & (GET_SLACK|GET_RESERVE);
			}
			else if (alliedunit(v, f, HELP_MONEY)) mask = (mode >> 6) & (GET_SLACK|GET_RESERVE);
			else continue;
			use -= new_use_pooled(v, rtype, mask, use);
		}
	}
	return count-use;
}

int
get_resource(const unit * u, resource_t res)
{
	return new_get_resource(u, oldresourcetype[res]);
}

int
change_resource(unit * u, resource_t res, int change)
{
	int i = 0;
	const item_type * itype = resource2item(oldresourcetype[res]);
	race_t urc = old_race(u->race);

	if (res==R_STONE && urc==RC_STONEGOLEM) {
	  i = u->number - (change+GOLEM_STONE-1)/GOLEM_STONE;
		scale_number(u, i);
	}
	else if (res==R_IRON && urc==RC_IRONGOLEM) {
	  i = u->number - (change+GOLEM_IRON-1)/GOLEM_IRON;
		scale_number(u, i);
	}
	else if (itype!=NULL) {
		item * it = i_change(&u->items, itype, change);
		if (it==NULL) return 0;
		return it->number;
	}
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
	const resource_type * rtype = oldresourcetype[resource];
	return new_get_resvalue(u, rtype);
}

static int
set_resvalue(unit * u, resource_t resource, int value)
{
	return new_set_resvalue(u, oldresourcetype[resource], value);
}

int
change_resvalue(unit * u, resource_t resource, int value)
{
	return set_resvalue(u, resource, get_resvalue(u, resource) + value);
}

int
get_reserved(const unit * u, resource_t resource)
{
	return new_get_pooled(u, oldresourcetype[resource], GET_RESERVE);
}

int
use_reserved(unit * u, resource_t resource, int count)
{
	return new_use_pooled(u, oldresourcetype[resource], GET_RESERVE, count);
}

int
get_slack(const unit * u, resource_t resource)
{
	return new_get_pooled(u, oldresourcetype[resource], GET_SLACK);
}

int
use_slack(unit * u, resource_t resource, int count)
{
	return new_use_pooled(u, oldresourcetype[resource], GET_SLACK, count);
}


int
get_pooled(const unit * u, const region * r, resource_t resource)
{
	return new_get_pooled(u, oldresourcetype[resource], GET_DEFAULT);
}


int
use_pooled(unit * u, region * r, resource_t resource, int count)
{
	return new_use_pooled(u, oldresourcetype[resource], GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, count);
}

int
use_pooled_give(unit * u, region * r, resource_t resource, int count)
{
	int use = count;
	use -= new_use_pooled(u, oldresourcetype[resource], GET_SLACK, use);
	if (use>0) use -= new_use_pooled(u, oldresourcetype[resource], GET_RESERVE|GET_POOLED_SLACK, use);
	return count-use;
}

int
get_all(const unit * u, const resource_type * rtype)
{
  return new_get_pooled(u, rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK|GET_POOLED_RESERVE|GET_POOLED_FORCE);
}

int
use_all(unit * u, const resource_type * rtype, int count)
{
  return new_use_pooled(u, rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK|GET_POOLED_RESERVE|GET_POOLED_FORCE, count);
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
			order * ord;
      for (ord=u->orders; ord; ord=ord->next) {
				if (u->number > 0 && (urace(u)->ec_flags & GETITEM) && get_keyword(ord) == K_RESERVE) {
          int use, count = geti();
          const resource_type * rtype;

          init_tokens(ord);
          skip_token();
          count = geti();

          rtype = findresourcetype(getstrtoken(), u->faction->locale);
          if (rtype == NULL) continue;

					new_set_resvalue(u, rtype, 0);	/* make sure the pool is empty */
					use = new_use_pooled(u, rtype, GET_DEFAULT, count);
					if (use) {
						new_set_resvalue(u, rtype, use);
						new_change_resource(u, rtype, use);
					}
				}
			}
		}
	}
}

