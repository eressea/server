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

#include <platform.h>
#include <kernel/config.h>
#include "curse.h"

/* kernel includes */
#include "building.h"
#include "faction.h"
#include "magic.h"
#include "message.h"
#include "objtypes.h"
#include "race.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "unit.h"
#include "version.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/nrmessage.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/storage.h>
#include <util/variant.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>

#include <tests.h>

#define MAXENTITYHASH 7919
curse *cursehash[MAXENTITYHASH];

void
c_setflag(curse *c, unsigned int flags)
{
  assert(c);
  c->flags = (c->flags & ~flags) | (flags & (c->type->flags ^ flags));
}
/* -------------------------------------------------------------------------- */
void
c_clearflag(curse *c, unsigned int flags)
{
  assert(c);
  c->flags = (c->flags & ~flags) | (c->type->flags & flags);
}

void
chash(curse *c)
{
  curse *old = cursehash[c->no %MAXENTITYHASH];

  cursehash[c->no %MAXENTITYHASH] = c;
  c->nexthash = old;
}

static void
cunhash(curse *c)
{
  curse **show;

  for (show = &cursehash[c->no % MAXENTITYHASH]; *show; show = &(*show)->nexthash) {
    if ((*show)->no == c->no)
      break;
  }
  if (*show) {
    assert(*show == c);
    *show = (*show)->nexthash;
    c->nexthash = 0;
  }
}

curse *
cfindhash(int i)
{
  curse *old;

  for (old = cursehash[i % MAXENTITYHASH]; old; old = old->nexthash)
    if (old->no == i)
      return old;
  return NULL;
}

/* ------------------------------------------------------------- */
/* at_curse */
void
curse_init(attrib * a) {
  a->data.v = calloc(1, sizeof(curse));
}

int
curse_age(attrib * a)
{
  curse * c = (curse*)a->data.v;
  int result = 0;

  if (c_flags(c) & CURSE_NOAGE) {
    c->duration = INT_MAX;
  }
  if (c->type->age) {
    result = c->type->age(c);
  }
  if (result!=0) {
    c->duration = 0;
  } else if (c->duration!=INT_MAX) {
    c->duration = MAX(0, c->duration-1);
  }
  return c->duration;
}

void
destroy_curse(curse * c)
{
  cunhash(c);
  free(c);
}

void
curse_done(attrib * a) {
  destroy_curse((curse *)a->data.v);
}

/** reads curses that have been removed from the code */
static int
read_ccompat(const char * cursename, struct storage * store)
{
  struct compat {
    const char * name;
    const char * tokens;
  } * seek, old_curses[] = { {"disorientationzone", ""}, {"shipdisorientation", ""}, { NULL, NULL } } ;
  for (seek=old_curses;seek->name;++seek) {
    if (strcmp(seek->tokens, cursename)==0) {
      const char * p;
      for (p=seek->name;p;++p) {
        switch (*p) {
          case 'd': store->r_int(store); break;
          case 's': store->r_str(store); break;
          case 't': store->r_tok(store); break;
          case 'i': store->r_id(store); break;
          case 'f': store->r_flt(store); break;
        }
      }
      return 0;
    }
  }
  return -1;
}

int
curse_read(attrib * a, void * owner, struct storage * store)
{
  curse * c = (curse*)a->data.v;
  int ur;
  char cursename[64];
  unsigned int flags;

  c->no = store->r_int(store);
  chash(c);
  store->r_tok_buf(store, cursename, sizeof(cursename));
  flags = store->r_int(store);
  c->duration = store->r_int(store);
  if (store->version >= CURSEVIGOURISFLOAT_VERSION) {
    c->vigour = store->r_flt(store);
  } else {
    int vigour = store->r_int(store);
    c->vigour = vigour;
  }
  if (store->version<INTPAK_VERSION) {
    ur = read_reference(&c->magician, store, read_int, resolve_unit);
  } else {
    ur = read_reference(&c->magician, store, read_unit_reference, resolve_unit);
  }
  if (store->version<CURSEFLOAT_VERSION) {
    c->effect = (double)store->r_int(store);
  } else {
    c->effect = store->r_flt(store);
  }
  c->type = ct_find(cursename);
  if (c->type==NULL) {
    int result = read_ccompat(cursename, store);
    if (result!=0) {
      log_error(("missing curse %s, no compatibility code either.\n", cursename));
    }
    assert(result==0);
    return AT_READ_FAIL;
  }
  if (store->version < CURSEFLAGS_VERSION) {
    c_setflag(c, flags);
  } else {
    c->flags = flags;
  }
  c_clearflag(c, CURSE_ISNEW);
  
  if (c->type->read) c->type->read(store, c, owner);
  else if (c->type->typ==CURSETYP_UNIT) {
    c->data.i = store->r_int(store);
  }
  if (c->type->typ == CURSETYP_REGION) {
    int rr = read_reference(&c->data.v, store, read_region_reference, RESOLVE_REGION(store->version));
    if (ur==0 && rr==0 && !c->data.v) {
      return AT_READ_FAIL;
    }
  }

  return AT_READ_OK;
}

void
curse_write(const attrib * a, const void * owner, struct storage * store)
{
  unsigned int flags;
  curse * c = (curse*)a->data.v;
  const curse_type * ct = c->type;
  unit * mage = (c->magician && c->magician->number)?c->magician:NULL;

  /* copied from c_clearflag */
  flags = (c->flags & ~CURSE_ISNEW) | (c->type->flags & CURSE_ISNEW);

  store->w_int(store, c->no);
  store->w_tok(store, ct->cname);
  store->w_int(store, flags);
  store->w_int(store, c->duration);
  store->w_flt(store, (float)c->vigour);
  write_unit_reference(mage, store);
  store->w_flt(store, (float)c->effect);

  if (c->type->write) c->type->write(store, c, owner);
  else if (c->type->typ == CURSETYP_UNIT) {
    store->w_int(store, c->data.i);
  }
  if (c->type->typ == CURSETYP_REGION) {
    write_region_reference((region*)c->data.v, store);
  }
}

attrib_type at_curse =
{
  "curse",
  curse_init,
  curse_done,
  curse_age,
  curse_write,
  curse_read,
  ATF_CURSE
};
/* ------------------------------------------------------------- */
/* Spruch identifizieren */

#include <util/umlaut.h>

typedef struct cursetype_list {
  struct cursetype_list * next;
  const curse_type * type;
} cursetype_list;

cursetype_list * cursetypes[256];

void
ct_register(const curse_type * ct)
{
  unsigned int hash = tolower(ct->cname[0]);
  cursetype_list ** ctlp = &cursetypes[hash];

  while (*ctlp) {
    cursetype_list * ctl = *ctlp;
    if (ctl->type==ct) return;
    ctlp=&ctl->next;
  }
  *ctlp = calloc(1, sizeof(cursetype_list));
  (*ctlp)->type = ct;
}

const curse_type *
ct_find(const char *c)
{
  unsigned int hash = tolower(c[0]);
  cursetype_list * ctl = cursetypes[hash];
  while (ctl) {
    if (strcmp(c, ctl->type->cname)==0) {
      return ctl->type;
    } else {
      size_t k = MIN(strlen(c), strlen(ctl->type->cname));
      if (!strncasecmp(c, ctl->type->cname, k)) {
        return ctl->type;
      }
    }
    ctl = ctl->next;
  }
  return NULL;
}

/* ------------------------------------------------------------- */
/* get_curse identifiziert eine Verzauberung �ber die ID und gibt
 * einen pointer auf die struct zur�ck.
 */

boolean
cmp_curse(const attrib * a, const void * data) {
  const curse * c = (const curse*)data;
  if (a->type->flags & ATF_CURSE) {
    if (!data || c == (curse*)a->data.v) return true;
  }
  return false;
}

boolean
cmp_cursetype(const attrib * a, const void * data)
{
  const curse_type * ct = (const curse_type *)data;
  if (a->type->flags & ATF_CURSE) {
    if (!data || ct == ((curse*)a->data.v)->type) return true;
  }
  return false;
}

curse *
get_cursex(attrib *ap, const curse_type * ctype, variant data, boolean(*compare)(const curse *, variant))
{
  attrib * a = a_select(ap, ctype, cmp_cursetype);
  while (a) {
    curse * c = (curse*)a->data.v;
    if (compare(c, data)) return c;
    a = a_select(a->next, ctype, cmp_cursetype);
  }
  return NULL;
}

curse *
get_curse(attrib *ap, const curse_type * ctype)
{
  attrib * a = ap;
  while (a) {
    if (a->type->flags & ATF_CURSE) {
      const attrib_type * at = a->type;
      while (a && a->type==at) {
        curse* c = (curse *)a->data.v;
        if (c->type==ctype) return c;
        a = a->next;
      }
    } else {
      a = a->nexttype;
    }
  }
  return NULL;
}

/* ------------------------------------------------------------- */
/* findet einen curse global anhand seiner 'curse-Einheitnummer' */

curse *
findcurse(int cid)
{
  return cfindhash(cid);
}

/* ------------------------------------------------------------- */
void
remove_curse(attrib **ap, const curse *c)
{
  attrib *a = a_select(*ap, c, cmp_curse);
  if (a) a_remove(ap, a);
}

/* gibt die allgemeine St�rke der Verzauberung zur�ck. id2 wird wie
 * oben benutzt. Dies ist nicht die Wirkung, sondern die Kraft und
 * damit der gegen Antimagie wirkende Widerstand einer Verzauberung */
static double
get_cursevigour(const curse *c)
{
  if (c) return c->vigour;
  return 0;
}

/* setzt die St�rke der Verzauberung auf i */
static void
set_cursevigour(curse *c, double vigour)
{
  assert(c && vigour > 0);
  c->vigour = vigour;
}

/* ver�ndert die St�rke der Verzauberung um +i und gibt die neue
 * St�rke zur�ck. Sollte die Zauberst�rke unter Null sinken, l�st er
 * sich auf.
 */
double
curse_changevigour(attrib **ap, curse *c, double vigour)
{
  vigour += get_cursevigour(c);

  if (vigour <= 0) {
    remove_curse(ap, c);
    vigour = 0;
  } else {
    set_cursevigour(c, vigour);
  }
  return vigour;
}

/* ------------------------------------------------------------- */

double
curse_geteffect(const curse *c)
{
  if (c==NULL) return 0;
  if (c_flags(c) & CURSE_ISNEW) return 0;
  return c->effect;
}

int
curse_geteffect_int(const curse *c)
{
  double effect = curse_geteffect(c);
  assert(effect-(int)effect == 0);
  return (int)effect;
}

/* ------------------------------------------------------------- */
static void
set_curseingmagician(struct unit *magician, struct attrib *ap_target, const curse_type *ct)
{
  curse * c = get_curse(ap_target, ct);
  if (c) {
    c->magician = magician;
  }
}

/* ------------------------------------------------------------- */
/* gibt bei Personenbeschr�nkten Verzauberungen die Anzahl der
 * betroffenen Personen zur�ck. Ansonsten wird 0 zur�ckgegeben. */
int
get_cursedmen(unit *u, const curse *c)
{
  int cursedmen = u->number;

  if (!c) return 0;

  /* je nach curse_type andere data struct */
  if (c->type->typ == CURSETYP_UNIT) {
    cursedmen = c->data.i;
  }

  return MIN(u->number, cursedmen);
}

/* setzt die Anzahl der betroffenen Personen auf cursedmen */
static void
set_cursedmen(curse *c, int cursedmen)
{
  if (!c) return;

  /* je nach curse_type andere data struct */
  if (c->type->typ == CURSETYP_UNIT) {
    c->data.i = cursedmen;
  }
}

/* ------------------------------------------------------------- */
/* Legt eine neue Verzauberung an. Sollte es schon einen Zauber
 * dieses Typs geben, gibt es den bestehenden zur�ck.
 */
static curse *
make_curse(unit *mage, attrib **ap, const curse_type *ct, double vigour,
    int duration, double effect, int men)
{
  curse *c;
  attrib * a;

  a = a_new(&at_curse);
  a_add(ap, a);
  c = (curse*)a->data.v;

  c->type = ct;
  c->flags = 0;
  c->vigour = vigour;
  c->duration = duration;
  c->effect = effect;
  c->magician = mage;

  c->no = newunitid();
  chash(c);

  switch (c->type->typ) {
    case CURSETYP_NORM:
      break;

    case CURSETYP_UNIT:
    {
      c->data.i = men;
      break;
    }

  }
  return c;
}


/* Mapperfunktion f�r das Anlegen neuer curse. Automatisch wird zum
 * passenden Typ verzweigt und die relevanten Variablen weitergegeben.
 */
curse *
create_curse(unit *magician, attrib **ap, const curse_type *ct, double vigour,
    int duration, double effect, int men)
{
  curse *c;

  /* die Kraft eines Spruchs darf nicht 0 sein*/
  assert(vigour > 0);

  c = get_curse(*ap, ct);

  if (c && (c_flags(c) & CURSE_ONLYONE)){
    return NULL;
  }
  assert(c==NULL || ct==c->type);

  /* es gibt schon eins diese Typs */
  if (c && ct->mergeflags != NO_MERGE) {
    if(ct->mergeflags & M_DURATION){
      c->duration = MAX(c->duration, duration);
    }
    if(ct->mergeflags & M_SUMDURATION){
      c->duration += duration;
    }
    if(ct->mergeflags & M_SUMEFFECT){
      c->effect += effect;
    }
    if(ct->mergeflags & M_MAXEFFECT){
      c->effect = MAX(c->effect, effect);
    }
    if(ct->mergeflags & M_VIGOUR){
      c->vigour = MAX(vigour, c->vigour);
    }
    if(ct->mergeflags & M_VIGOUR_ADD){
      c->vigour = vigour + c->vigour;
    }
    if(ct->mergeflags & M_MEN){
      switch (ct->typ) {
        case CURSETYP_UNIT:
        {
          c->data.i += men;
        }
      }
    }
    set_curseingmagician(magician, *ap, ct);
  } else {
    c = make_curse(magician, ap, ct, vigour, duration, effect, men);
  }
  return c;
}

/* ------------------------------------------------------------- */
/* hier m�ssen alle c-typen, die auf Einheiten gezaubert werden k�nnen,
 * ber�cksichtigt werden */

static void
do_transfer_curse(curse *c, unit * u, unit * u2, int n)
{
  int cursedmen = 0;
  int men = get_cursedmen(u, c);
  boolean dogive = false;
  const curse_type *ct = c->type;

  switch ((ct->flags | c->flags) & CURSE_SPREADMASK) {
    case CURSE_SPREADALWAYS:
      dogive = true;
      men = u2->number + n;
      break;

    case CURSE_SPREADMODULO:
    {
      int i;
      int u_number = u->number;
      for (i=0;i<n+1 && u_number>0;i++){
        if (rng_int()%u_number < cursedmen){
          ++men;
          --cursedmen;
          dogive = true;
        }
        --u_number;
      }
      break;
    }
    case CURSE_SPREADCHANCE:
      if (chance(u2->number/(double)(u2->number + n))) {
        men = u2->number + n;
        dogive = true;
      }
      break;
    case CURSE_SPREADNEVER:
      break;
  }

  if (dogive == true) {
    curse * cnew = make_curse(c->magician, &u2->attribs, c->type, c->vigour,
      c->duration, c->effect, men);
    cnew->flags = c->flags;

    if (ct->typ == CURSETYP_UNIT) set_cursedmen(cnew, men);
  }
}

void
transfer_curse(unit * u, unit * u2, int n)
{
  attrib * a;

  a = a_find(u->attribs, &at_curse);
  while (a && a->type==&at_curse) {
    curse *c = (curse*)a->data.v;
    do_transfer_curse(c, u, u2, n);
    a = a->next;
  }
}

/* ------------------------------------------------------------- */

boolean
curse_active(const curse *c)
{
  if (!c) return false;
  if (c_flags(c) & CURSE_ISNEW) return false;
  if (c->vigour <= 0) return false;

  return true;
}

boolean
is_cursed_internal(attrib *ap, const curse_type *ct)
{
  curse *c = get_curse(ap, ct);

  if (!c)
    return false;

  return true;
}


boolean
is_cursed_with(const attrib *ap, const curse *c)
{
  const attrib *a = ap;

  while (a) {
    if ((a->type->flags & ATF_CURSE) && (c == (const curse *)a->data.v)) {
      return true;
    }
    a = a->next;
  }

  return false;
}
/* ------------------------------------------------------------- */
/* cursedata */
/* ------------------------------------------------------------- */
/*
 * typedef struct curse_type {
 *  const char *cname; (Name der Zauberwirkung, Identifizierung des curse)
 *  int typ;
 *  spread_t spread;
 *  unsigned int mergeflags;
 *  int (*curseinfo)(const struct locale*, const void*, int, curse*, int);
 *  void (*change_vigour)(curse*, double);
 *  int (*read)(struct storage * store, curse * c);
 *  int (*write)(struct storage * store, const curse * c);
 * } curse_type;
 */

int
resolve_curse(variant id, void * address)
{
  int result = 0;
  curse * c = NULL;
  if (id.i!=0) {
    c = cfindhash(id.i);
    if (c==NULL) {
      result = -1;
    }
  }
  *(curse**)address = c;
  return result;
}

static const char * oldnames[MAXCURSE] = {
  /* OBS: when removing curses, remember to update read_ccompat() */
  "fogtrap",
  "antimagiczone",
  "farvision",
  "gbdream",
  "auraboost",
  "maelstrom",
  "blessedharvest",
  "drought",
  "badlearn",
  "stormwind",
  "flyingship",
  "nodrift",
  "depression",
  "magicwalls",
  "strongwall",
  "astralblock",
  "generous",
  "peacezone",
  "magicstreet",
  "magicrunes",
  "badmagicresistancezone",
  "goodmagicresistancezone",
  "slavery",
  "calmmonster",
  "oldrace",
  "fumble",
  "riotzone",
  "nocostbuilding",
  "godcursezone",
  "speed",
  "orcish",
  "magicboost",
  "insectfur",
  "strength",
  "worse",
  "magicresistance",
  "itemcloak",
  "sparkle",
  "skillmod"
};

const char *
oldcursename(int id)
{
  return oldnames[id];
}

/* ------------------------------------------------------------- */
message *
cinfo_simple(const void * obj, typ_t typ, const struct curse *c, int self)
{
  struct message * msg;

  unused(typ);
  unused(self);
  unused(obj);

  msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
  if (msg==NULL) {
    log_error(("There is no curseinfo for %s.\n", c->type->cname));
  }
  return msg;
}


/* ------------------------------------------------------------- */
/* Antimagie - curse aufl�sen */
/* ------------------------------------------------------------- */

/* Wenn der Curse schw�cher ist als der cast_level, dann wird er
* aufgel�st, bzw seine Kraft (vigour) auf 0 gesetzt.
* Ist der cast_level zu gering, hat die Antimagie nur mit einer Chance
* von 100-20*Stufenunterschied % eine Wirkung auf den Curse. Dann wird
* die Kraft des Curse um die halbe St�rke der Antimagie reduziert.
* Zur�ckgegeben wird der noch unverbrauchte Rest von force.
*/
double
destr_curse(curse* c, int cast_level, double force)
{
  if (cast_level < c->vigour) { /* Zauber ist nicht stark genug */
    double probability = 0.1 + (cast_level - c->vigour)*0.2;
    /* pro Stufe Unterschied -20% */
    if (chance(probability)) {
      force -= c->vigour;
      if (c->type->change_vigour) {
        c->type->change_vigour(c, -(cast_level+1/2));
      } else {
        c->vigour -= cast_level+1/2;
      }
    }
  } else { /* Zauber ist st�rker als curse */
    if (force >= c->vigour) { /* reicht die Kraft noch aus? */
      force -= c->vigour;
      if (c->type->change_vigour) {
        c->type->change_vigour(c, -c->vigour);
      } else {
        c->vigour = 0;
      }

    }
  }
  return force;
}

#ifndef DISABLE_TESTS
#include "curse_test.c"
#endif
