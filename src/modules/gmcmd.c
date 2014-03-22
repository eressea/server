/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>
#include "gmcmd.h"
#include <kernel/command.h>

/* misc includes */
#include <items/demonseye.h>
#include <attributes/key.h>
#include <triggers/gate.h>
#include <triggers/unguard.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/reports.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/message.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/umlaut.h>
#include <util/parser.h>
#include <util/rng.h>

#include <storage.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

/**
 ** at_permissions
 **/

static void mistake(const unit * u, struct order *ord, const char *comment)
{
  if (!is_monsters(u->faction)) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "mistake",
        "error", comment));
  }
}

static void
write_permissions(const attrib * a, const void *owner, struct storage *store)
{
  a_write(store, (attrib *) a->data.v, owner);
}

static int read_permissions(attrib * a, void *owner, struct storage *store)
{
  attrib *attr = NULL;
  a_read(store, &attr, NULL);
  a->data.v = attr;
  return AT_READ_OK;
}

struct attrib_type at_permissions = {
  "GM:permissions",
  NULL, NULL, NULL,
  write_permissions, read_permissions,
  ATF_UNIQUE
};

attrib *make_atpermissions(void)
{
  return a_new(&at_permissions);
}

/**
 ** GM: CREATE <number> <itemtype>
 **/

static void
write_gmcreate(const attrib * a, const void *owner, struct storage *store)
{
  const item_type *itype = (const item_type *)a->data.v;
  assert(itype);
  WRITE_TOK(store, resourcename(itype->rtype, 0));
}

static int read_gmcreate(attrib * a, void *owner, struct storage *store)
{
  char zText[32];
  READ_TOK(store, zText, sizeof(zText));
  a->data.v = it_find(zText);
  if (a->data.v == NULL) {
    log_error("unknown itemtype %s in gmcreate attribute\n", zText);
    return AT_READ_FAIL;
  }
  return AT_READ_OK;
}

/* at_gmcreate specifies that the owner can create items of a particular type */
attrib_type at_gmcreate = {
  "GM:create",
  NULL, NULL, NULL,
  write_gmcreate, read_gmcreate
};

attrib *make_atgmcreate(const struct item_type * itype)
{
  attrib *a = a_new(&at_gmcreate);
  a->data.v = (void *)itype;
  return a;
}

static void gm_create(const void *tnext, struct unit *u, struct order *ord)
{
  int i;
  attrib *permissions = a_find(u->faction->attribs, &at_permissions);
  if (permissions)
    permissions = (attrib *) permissions->data.v;
  if (!permissions)
    return;
  i = getint();

  if (i > 0) {
    const char *iname = getstrtoken();
    const item_type *itype = finditemtype(iname, u->faction->locale);
    if (itype == NULL) {
      mistake(u, ord, "unknown item.");
    } else {
      attrib *a = a_find(permissions, &at_gmcreate);

      while (a && a->type == &at_gmcreate && a->data.v != (void *)itype)
        a = a->next;
      if (a)
        i_change(&u->items, itype, i);
      else
        mistake(u, ord, "your faction cannot create this item.");
    }
  }
}

static bool has_permission(const attrib * permissions, unsigned int key)
{
  return (find_key((attrib *) permissions->data.v, key) ||
    find_key((attrib *) permissions->data.v, atoi36("master")));
}

/**
 ** GM: GATE <id> <x> <y>
 ** requires: permission-key "gmgate"
 **/
static void gm_gate(const void *tnext, struct unit * u, struct order *ord)
{
  const struct plane *pl = rplane(u->region);
  int id = getid();
  int x = rel_to_abs(pl, u->faction, getint(), 0);
  int y = rel_to_abs(pl, u->faction, getint(), 1);
  building *b = findbuilding(id);
  region *r;

  pnormalize(&x, &y, pl);
  r = findregion(x, y);
  if (b == NULL || r == NULL || pl != rplane(b->region) || pl != rplane(r)) {
    mistake(u, ord, "the unit cannot transform this building.");
    return;
  } else {
    /* checking permissions */
    attrib *permissions = a_find(u->faction->attribs, &at_permissions);
    if (permissions && has_permission(permissions, atoi36("gmgate"))) {
      remove_triggers(&b->attribs, "timer", &tt_gate);
      remove_triggers(&b->attribs, "create", &tt_unguard);
      if (r != b->region) {
        add_trigger(&b->attribs, "timer", trigger_gate(b, r));
        add_trigger(&b->attribs, "create", trigger_unguard(b));
        fset(b, BLD_UNGUARDED);
      }
    }
  }
}

/**
 ** GM: TERRAFORM <x> <y> <terrain>
 ** requires: permission-key "gmterf"
 **/
static void gm_terraform(const void *tnext, struct unit *u, struct order *ord)
{
  const struct plane *p = rplane(u->region);
  int x = rel_to_abs(p, u->faction, getint(), 0);
  int y = rel_to_abs(p, u->faction, getint(), 1);
  const char *c = getstrtoken();
  variant token;
  void **tokens = get_translations(u->faction->locale, UT_TERRAINS);
  region *r;
  pnormalize(&x, &y, p);
  r = findregion(x, y);

  if (r == NULL || p != rplane(r)) {
    mistake(u, ord, "region is in another plane.");
    return;
  } else {
    /* checking permissions */
    attrib *permissions = a_find(u->faction->attribs, &at_permissions);
    if (!permissions || !has_permission(permissions, atoi36("gmterf")))
      return;
  }

  if (findtoken(*tokens, c, &token) != E_TOK_NOMATCH) {
    const terrain_type *terrain = (const terrain_type *)token.v;
    terraform_region(r, terrain);
  }
}

/**
 ** GM: TELEPORT <unit> <x> <y>
 ** requires: permission-key "gmtele"
 **/
static void gm_teleport(const void *tnext, struct unit *u, struct order *ord)
{
  const struct plane *p = rplane(u->region);
  unit *to = findunit(getid());
  int x = rel_to_abs(p, u->faction, getint(), 0);
  int y = rel_to_abs(p, u->faction, getint(), 1);
  region *r = findregion(x, y);

  if (r == NULL || p != rplane(r)) {
    mistake(u, ord, "region is in another plane.");
  } else if (to == NULL) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
        ""));
  } else if (rplane(to->region) != rplane(r) && !ucontact(to, u)) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_no_contact",
        "target", to));
  } else {
    /* checking permissions */
    attrib *permissions = a_find(u->faction->attribs, &at_permissions);
    if (!permissions || !has_permission(permissions, atoi36("gmtele"))) {
      mistake(u, ord, "permission denied.");
    } else
      move_unit(to, r, NULL);
  }
}

/**
 ** GM: TELL PLANE <string>
 ** requires: permission-key "gmmsgr"
 **/
static void gm_messageplane(const void *tnext, struct unit *gm, struct order *ord)
{
  const struct plane *p = rplane(gm->region);
  const char *zmsg = getstrtoken();
  if (p == NULL) {
    mistake(gm, ord, "In diese Ebene kann keine Nachricht gesandt werden.");
  } else {
    /* checking permissions */
    attrib *permissions = a_find(gm->faction->attribs, &at_permissions);
    if (!permissions || !has_permission(permissions, atoi36("gmmsgr"))) {
      mistake(gm, ord, "permission denied.");
    } else {
      message *msg = msg_message("msg_event", "string", zmsg);
      faction *f;
      region *r;
      for (f = factions; f; f = f->next) {
        freset(f, FFL_SELECT);
      }
      for (r = regions; r; r = r->next) {
        unit *u;
        if (rplane(r) != p)
          continue;
        for (u = r->units; u; u = u->next)
          if (!fval(u->faction, FFL_SELECT)) {
            f = u->faction;
            fset(f, FFL_SELECT);
            add_message(&f->msgs, msg);
          }
      }
      msg_release(msg);
    }
  }
}

static void
gm_messagefaction(const void *tnext, struct unit *gm, struct order *ord)
{
  int n = getid();
  faction *f = findfaction(n);
  const char *msg = getstrtoken();
  plane *p = rplane(gm->region);
  attrib *permissions = a_find(gm->faction->attribs, &at_permissions);
  if (!permissions || !has_permission(permissions, atoi36("gmmsgr"))) {
    mistake(gm, ord, "permission denied.");
    return;
  }
  if (f != NULL) {
    region *r;
    for (r = regions; r; r = r->next)
      if (rplane(r) == p) {
        unit *u;
        for (u = r->units; u; u = u->next)
          if (u->faction == f) {
            add_message(&f->msgs, msg_message("msg_event", "string", msg));
            return;
          }
      }
  }
  mistake(gm, ord, "cannot send messages to this faction.");
}

/**
 ** GM: TELL REGION <x> <y> <string>
 ** requires: permission-key "gmmsgr"
 **/
static void gm_messageregion(const void *tnext, struct unit *u, struct order *ord)
{
  const struct plane *p = rplane(u->region);
  int x = rel_to_abs(p, u->faction, getint(), 0);
  int y = rel_to_abs(p, u->faction, getint(), 1);
  const char *msg = getstrtoken();
  region *r = findregion(x, y);

  if (r == NULL || p != rplane(r)) {
    mistake(u, ord, "region is in another plane.");
  } else {
    /* checking permissions */
    attrib *permissions = a_find(u->faction->attribs, &at_permissions);
    if (!permissions || !has_permission(permissions, atoi36("gmmsgr"))) {
      mistake(u, ord, "permission denied.");
    } else {
      add_message(&r->msgs, msg_message("msg_event", "string", msg));
    }
  }
}

/**
 ** GM: KILL UNIT <id> <string>
 ** requires: permission-key "gmkill"
 **/
static void gm_killunit(const void *tnext, struct unit *u, struct order *ord)
{
  const struct plane *p = rplane(u->region);
  unit *target = findunit(getid());
  const char *msg = getstrtoken();
  region *r = target->region;

  if (r == NULL || p != rplane(r)) {
    mistake(u, ord, "region is in another plane.");
  } else {
    /* checking permissions */
    attrib *permissions = a_find(u->faction->attribs, &at_permissions);
    if (!permissions || !has_permission(permissions, atoi36("gmkill"))) {
      mistake(u, ord, "permission denied.");
    } else {
      scale_number(target, 0);
      ADDMSG(&target->faction->msgs, msg_message("killedbygm",
          "region unit string", r, target, msg));
    }
  }
}

/**
 ** GM: KILL FACTION <id> <string>
 ** requires: permission-key "gmmsgr"
 **/
static void gm_killfaction(const void *tnext, struct unit *u, struct order *ord)
{
  int n = getid();
  faction *f = findfaction(n);
  const char *msg = getstrtoken();
  plane *p = rplane(u->region);
  attrib *permissions = a_find(u->faction->attribs, &at_permissions);
  if (!permissions || !has_permission(permissions, atoi36("gmkill"))) {
    mistake(u, ord, "permission denied.");
    return;
  }
  if (f != NULL) {
    region *r;
    for (r = regions; r; r = r->next)
      if (rplane(r) == p) {
        unit *target;
        for (target = r->units; target; target = target->next) {
          if (target->faction == f) {
            scale_number(target, 0);
            ADDMSG(&target->faction->msgs, msg_message("killedbygm",
                "region unit string", r, target, msg));
            return;
          }
        }
      }
  }
  mistake(u, ord, "cannot remove a unit from this faction.");
}

/**
 ** GM: TELL <unit> <string>
 ** requires: permission-key "gmmsgr"
 **/
static void gm_messageunit(const void *tnext, struct unit *u, struct order *ord)
{
  const struct plane *p = rplane(u->region);
  unit *target = findunit(getid());
  const char *msg = getstrtoken();
  region *r;

  if (target == NULL) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
        ""));
    return;
  }

  r = target->region;

  if (r == NULL || p != rplane(r)) {
    mistake(u, ord, "region is in another plane.");
  } else {
    /* checking permissions */
    attrib *permissions = a_find(u->faction->attribs, &at_permissions);
    if (!permissions || !has_permission(permissions, atoi36("gmmsgu"))) {
      mistake(u, ord, "permission denied.");
    } else {
      add_message(&target->faction->msgs,
        msg_message("regionmessage", "region sender string", r, u, msg));
    }
  }
}

/**
 ** GM: GIVE <unit> <int> <itemtype>
 ** requires: permission-key "gmgive"
 **/
static void gm_give(const void *tnext, struct unit *u, struct order *ord)
{
  unit *to = findunit(getid());
  int num = getint();
  const item_type *itype = finditemtype(getstrtoken(), u->faction->locale);

  if (to == NULL || rplane(to->region) != rplane(u->region)) {
    /* unknown or in another plane */
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
        ""));
  } else if (itype == NULL || i_get(u->items, itype) == 0) {
    /* unknown or not enough */
    mistake(u, ord, "invalid item or item not found.");
  } else {
    /* checking permissions */
    attrib *permissions = a_find(u->faction->attribs, &at_permissions);
    if (!permissions || !has_permission(permissions, atoi36("gmgive"))) {
      mistake(u, ord, "permission denied.");
    } else {
      int i = i_get(u->items, itype);
      if (i < num)
        num = i;
      if (num) {
        i_change(&u->items, itype, -num);
        i_change(&to->items, itype, num);
      }
    }
  }
}

/**
 ** GM: TAKE <unit> <int> <itemtype>
 ** requires: permission-key "gmtake"
 **/
static void gm_take(const void *tnext, struct unit *u, struct order *ord)
{
  unit *to = findunit(getid());
  int num = getint();
  const item_type *itype = finditemtype(getstrtoken(), u->faction->locale);

  if (to == NULL || rplane(to->region) != rplane(u->region)) {
    /* unknown or in another plane */
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
        ""));
  } else if (itype == NULL || i_get(to->items, itype) == 0) {
    /* unknown or not enough */
    mistake(u, ord, "invalid item or item not found.");
  } else {
    /* checking permissions */
    attrib *permissions = a_find(u->faction->attribs, &at_permissions);
    if (!permissions || !has_permission(permissions, atoi36("gmtake"))) {
      mistake(u, ord, "permission denied.");
    } else {
      int i = i_get(to->items, itype);
      if (i < num)
        num = i;
      if (num) {
        i_change(&to->items, itype, -num);
        i_change(&u->items, itype, num);
      }
    }
  }
}

/**
 ** GM: SKILL <unit> <skill> <tage>
 ** requires: permission-key "gmskil"
 **/
static void gm_skill(const void *tnext, struct unit *u, struct order *ord)
{
  unit *to = findunit(getid());
  skill_t skill = findskill(getstrtoken(), u->faction->locale);
  int num = getint();

  if (to == NULL || rplane(to->region) != rplane(u->region)) {
    /* unknown or in another plane */
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
        ""));
  } else if (skill == NOSKILL || skill == SK_MAGIC || skill == SK_ALCHEMY) {
    /* unknown or not enough */
    mistake(u, ord, "unknown skill, or skill cannot be raised.");
  } else if (num < 0 || num > 30) {
    /* sanity check failed */
    mistake(u, ord, "invalid value.");
  } else {
    /* checking permissions */
    attrib *permissions = a_find(u->faction->attribs, &at_permissions);
    if (!permissions || !has_permission(permissions, atoi36("gmskil"))) {
      mistake(u, ord, "permission denied.");
    } else {
      set_level(to, skill, num);
    }
  }
}

static void * g_keys;
static void * g_root;
static void * g_tell;
static void * g_kill;

void register_gmcmd(void)
{
  at_register(&at_gmcreate);
  at_register(&at_permissions);
  add_command(&g_root, &g_keys, "gm", NULL);
  add_command(&g_keys, NULL, "terraform", &gm_terraform);
  add_command(&g_keys, NULL, "create", &gm_create);
  add_command(&g_keys, NULL, "gate", &gm_gate);
  add_command(&g_keys, NULL, "give", &gm_give);
  add_command(&g_keys, NULL, "take", &gm_take);
  add_command(&g_keys, NULL, "teleport", &gm_teleport);
  add_command(&g_keys, NULL, "skill", &gm_skill);
  add_command(&g_keys, &g_tell, "tell", NULL);
  add_command(&g_tell, NULL, "region", &gm_messageregion);
  add_command(&g_tell, NULL, "unit", &gm_messageunit);
  add_command(&g_tell, NULL, "plane", &gm_messageplane);
  add_command(&g_tell, NULL, "faction", &gm_messagefaction);
  add_command(&g_keys, &g_kill, "kill", NULL);
  add_command(&g_kill, NULL, "unit", &gm_killunit);
  add_command(&g_kill, NULL, "faction", &gm_killfaction);
}

/*
 * execute gm-commands for all units in the game
 */

void gmcommands(void)
{
  region **rp = &regions;
  while (*rp) {
    region *r = *rp;
    unit **up = &r->units;
    while (*up) {
      unit *u = *up;
      struct order *ord;
      for (ord = u->orders; ord; ord = ord->next) {
        if (get_keyword(ord) == K_GM) {
          do_command(&g_root, u, ord);
        }
      }
      if (u == *up)
        up = &u->next;
    }
    if (*rp == r)
      rp = &r->next;
  }
}

#define EXTENSION 10000

faction *gm_addquest(const char *email, const char *name, int radius,
  unsigned int flags)
{
  plane *pl;
  watcher *w = calloc(sizeof(watcher), 1);
  region *center;
  bool invalid = false;
  int minx, miny, maxx, maxy, cx, cy;
  int x;
  faction *f;

  /* GM playfield */
  do {
    minx = ((rng_int() % (2 * EXTENSION)) - EXTENSION);
    miny = ((rng_int() % (2 * EXTENSION)) - EXTENSION);
    for (x = 0; !invalid && x <= radius * 2; ++x) {
      int y;
      for (y = 0; !invalid && y <= radius * 2; ++y) {
        region *r = findregion(minx + x, miny + y);
        if (r)
          invalid = true;
      }
    }
  } while (invalid);
  maxx = minx + 2 * radius;
  cx = minx + radius;
  maxy = miny + 2 * radius;
  cy = miny + radius;
  pl = create_new_plane(rng_int(), name, minx, maxx, miny, maxy, flags);
  center = new_region(cx, cy, pl, 0);
  for (x = 0; x <= 2 * radius; ++x) {
    int y;
    for (y = 0; y <= 2 * radius; ++y) {
      region *r = findregion(minx + x, miny + y);
      if (!r) {
        r = new_region(minx + x, miny + y, pl, 0);
      }
      freset(r, RF_ENCOUNTER);
      if (distance(r, center) == radius) {
        terraform_region(r, newterrain(T_FIREWALL));
      } else if (r == center) {
        terraform_region(r, newterrain(T_PLAIN));
      } else {
        terraform_region(r, newterrain(T_OCEAN));
      }
    }
  }

  /* watcher: */
  f = gm_addfaction(email, pl, center);
  w->faction = f;
  w->mode = see_unit;
  w->next = pl->watchers;
  pl->watchers = w;

  return f;
}

faction *gm_addfaction(const char *email, plane * p, region * r)
{
  attrib *a;
  unit *u;
  faction *f = calloc(1, sizeof(faction));

  assert(p != NULL);

  /* GM faction */
  a_add(&f->attribs, make_key(atoi36("quest")));
  f->banner = _strdup("quest faction");
  f->name = _strdup("quest faction");
  f->passw = _strdup(itoa36(rng_int()));
  if (set_email(&f->email, email) != 0) {
    log_error("Invalid email address for faction %s: %s\n", itoa36(f->no), email);
  }
  f->race = new_race[RC_TEMPLATE];
  f->age = 0;
  f->lastorders = turn;
  f->alive = true;
  f->locale = default_locale;
  f->options =
    want(O_COMPRESS) | want(O_REPORT) | want(O_COMPUTER) | want(O_ADRESSEN);
  {
    faction *xist;
    int id = atoi36("gm00") - 1;
    do {
      xist = findfaction(++id);
    } while (xist);

    f->no = id;
    addlist(&factions, f);
    fhash(f);
  }

  /* generic permissions */
  a = a_add(&f->attribs, a_new(&at_permissions));
  if (a) {
    attrib *ap = (attrib *) a->data.v;
    const char *keys[] =
      { "gmterf", "gmtele", "gmgive", "gmskil", "gmtake", "gmmsgr", "gmmsgu",
        "gmgate", 0 };
    const char **key_p = keys;
    while (*key_p) {
      add_key(&ap, atoi36(*key_p));
      ++key_p;
    }
    a_add(&ap, make_atgmcreate(resource2item(r_silver)));

    a->data.v = ap;
  }

  /* one initial unit */
  u = create_unit(r, f, 1, new_race[RC_TEMPLATE], 1, "quest master", NULL);
  u->irace = new_race[RC_GNOME];

  return f;
}

plane *gm_addplane(int radius, unsigned int flags, const char *name)
{
  region *center;
  plane *pl;
  bool invalid = false;
  int minx, miny, maxx, maxy, cx, cy;
  int x;

  /* GM playfield */
  do {
    minx = (rng_int() % (2 * EXTENSION)) - EXTENSION;
    miny = (rng_int() % (2 * EXTENSION)) - EXTENSION;
    for (x = 0; !invalid && x <= radius * 2; ++x) {
      int y;
      for (y = 0; !invalid && y <= radius * 2; ++y) {
        region *r = findregion(minx + x, miny + y);
        if (r)
          invalid = true;
      }
    }
  } while (invalid);
  maxx = minx + 2 * radius;
  cx = minx + radius;
  maxy = miny + 2 * radius;
  cy = miny + radius;
  pl = create_new_plane(rng_int(), name, minx, maxx, miny, maxy, flags);
  center = new_region(cx, cy, pl, 0);
  for (x = 0; x <= 2 * radius; ++x) {
    int y;
    for (y = 0; y <= 2 * radius; ++y) {
      region *r = findregion(minx + x, miny + y);
      if (!r)
        r = new_region(minx + x, miny + y, pl, 0);
      freset(r, RF_ENCOUNTER);
      if (distance(r, center) == radius) {
        terraform_region(r, newterrain(T_FIREWALL));
      } else if (r == center) {
        terraform_region(r, newterrain(T_PLAIN));
      } else {
        terraform_region(r, newterrain(T_OCEAN));
      }
    }
  }
  return pl;
}
