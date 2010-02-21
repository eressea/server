/* vi: set ts=2:
 *
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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

#include <platform.h>
#include <kernel/config.h>
#include "pool.h"

#include "faction.h"
#include "item.h"
#include "magic.h"
#include "order.h"
#include "region.h"
#include "race.h"
#include "unit.h"

#include <util/parser.h>
#include <util/log.h>

#include <assert.h>
#include <stdlib.h>

#define TODO_POOL
#undef TODO_RESOURCES

static const race * rc_stonegolem;
static const race * rc_irongolem;

static void
init_static(void)
{
  static boolean init = false;
  if (!init) {
    init = true;
    rc_stonegolem = rc_find("stonegolem");
    if (rc_stonegolem==NULL) log_error(("Could not find race: stonegolem\n"));
    rc_irongolem = rc_find("irongolem");
    if (rc_irongolem==NULL) log_error(("Could not find race: irongolem\n"));
  }
}

int
get_resource(const unit * u, const resource_type * rtype)
{
  const item_type * itype = resource2item(rtype);

  if (rtype->uget) {
    /* this resource is probably special */
    int i = rtype->uget(u, rtype);
    if (i>=0) return i;
  }
  if (itype!=NULL) {
    if (!rc_stonegolem) init_static();
    if (itype == olditemtype[R_STONE] && u->race==rc_stonegolem) {
      return u->number*GOLEM_STONE;
    } else if (itype==olditemtype[R_IRON] && u->race==rc_irongolem) {
      return u->number*GOLEM_IRON;
    } else {
      const item * i = *i_findc(&u->items, itype);
      if (i) return i->number;
      return 0;
    }
  }
  if (rtype == oldresourcetype[R_AURA])
    return get_spellpoints(u);
  if (rtype == oldresourcetype[R_PERMAURA])
    return max_spellpoints(u->region, u);
  log_error(("trying to get unknown resource '%s'.\n", rtype->_name[0]));
  return 0;
}

int
change_resource(unit * u, const resource_type * rtype, int change)
{
  int i = 0;

  if (rtype->uchange)
    i = rtype->uchange(u, rtype, change);
  else if (rtype == oldresourcetype[R_AURA])
    i = change_spellpoints(u, change);
  else if (rtype == oldresourcetype[R_PERMAURA])
    i = change_maxspellpoints(u, change);
  else
    assert(!"undefined resource detected. rtype->uchange not initialized.");
  assert(i >= 0 && (i < 100000000));  /* soft test to catch mischief */
  return i;
}

int
get_reservation(const unit * u, const resource_type * rtype)
{
  struct reservation * res = u->reservations;

  if (!rc_stonegolem) init_static();

  if (rtype==oldresourcetype[R_STONE] && u->race==rc_stonegolem)
    return (u->number * GOLEM_STONE);
  if (rtype==oldresourcetype[R_IRON] && u->race==rc_irongolem)
    return (u->number * GOLEM_IRON);
  while (res && res->type!=rtype) res=res->next;
  if (res) return res->value;
  return 0;
}

int
change_reservation(unit * u, const resource_type * rtype, int value)
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
get_pooled(const unit * u, const resource_type * rtype, unsigned int mode, int count)
{
  const faction * f = u->faction;
  unit *v;
  int use = 0;
  region * r = u->region;
  int have = get_resource(u, rtype);

  if ((u->race->ec_flags & GETITEM) == 0) {
    mode &= (GET_SLACK|GET_RESERVE);
  }

  if ((mode & GET_SLACK) && (mode & GET_RESERVE)) use = have;
  else {
    int reserve = get_reservation(u, rtype);
    int slack = MAX(0, have-reserve);
    if (mode & GET_RESERVE) use = have-slack;
    else if (mode & GET_SLACK) use = slack;
  }
  if (rtype->flags & RTF_POOLED && mode & ~(GET_SLACK|GET_RESERVE)) {
    for (v = r->units; v && use<count; v = v->next) if (u!=v) {
      int mask;

      if (v->items==NULL && rtype->uget==NULL) continue;
      if ((urace(v)->ec_flags & GIVEITEM) == 0) continue;

      if (v->faction == f) {
        mask = (mode >> 3) & (GET_SLACK|GET_RESERVE);
      }
      else if (alliedunit(v, f, HELP_MONEY)) mask = (mode >> 6) & (GET_SLACK|GET_RESERVE);
      else continue;
      use += get_pooled(v, rtype, mask, count-use);
    }
  }
  return use;
}

int
use_pooled(unit * u, const resource_type * rtype, unsigned int mode, int count)
{
  const faction *f = u->faction;
  unit *v;
  int use = count;
  region * r = u->region;
  int n = 0, have = get_resource(u, rtype);

  if ((u->race->ec_flags & GETITEM) == 0) {
    mode &= (GET_SLACK|GET_RESERVE);
  }

  if ((mode & GET_SLACK) && (mode & GET_RESERVE)) {
    n = MIN(use, have);
  } else {
    int reserve = get_reservation(u, rtype);
    int slack = MAX(0, have-reserve);
    if (mode & GET_RESERVE) {
      n = have-slack;
      n = MIN(use, n);
      change_reservation(u, rtype, -n);
    }
    else if (mode & GET_SLACK) {
      n = MIN(use, slack);
    }
  }
  if (n>0) {
    change_resource(u, rtype, -n);
    use -= n;
  }

  if (rtype->flags & RTF_POOLED && mode & ~(GET_SLACK|GET_RESERVE)) {
    for (v = r->units; use>0 && v!=NULL; v = v->next) if (u!=v) {
      int mask;
      if ((urace(v)->ec_flags & GIVEITEM) == 0) continue;
      if (v->items==NULL && rtype->uget==NULL) continue;

      if (v->faction == f) {
        mask = (mode >> 3) & (GET_SLACK|GET_RESERVE);
      }
      else if (alliedunit(v, f, HELP_MONEY)) mask = (mode >> 6) & (GET_SLACK|GET_RESERVE);
      else continue;
      use -= use_pooled(v, rtype, mask, use);
    }
  }
  return count-use;
}


int
reserve_cmd(unit * u, struct order *ord)
{
  if (u->number > 0 && (urace(u)->ec_flags & GETITEM)) {
    int use, count;
    const resource_type * rtype;
    const char * s;

    init_tokens(ord);
    skip_token();
    s = getstrtoken();
    count = atoip((const char *)s);

    if (count == 0 && findparam(s, u->faction->locale)==P_EACH) {
      count = getint() * u->number;
    }

    rtype = findresourcetype(getstrtoken(), u->faction->locale);
    if (rtype == NULL) return 0;

    new_set_resvalue(u, rtype, 0);  /* make sure the pool is empty */
    use = use_pooled(u, rtype, GET_DEFAULT, count);
    if (use) {
      new_set_resvalue(u, rtype, use);
      change_resource(u, rtype, use);
    }
  }
  return 0;
}

