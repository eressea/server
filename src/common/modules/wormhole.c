/* vi: set ts=2:
 +-------------------+  
 |                   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 | Eressea PBEM host |  Enno Rehling <enno@eressea-pbem.de>
 | (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
 |                   |
 +-------------------+  

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>

#ifdef WORMHOLE_MODULE
#include "wormhole.h"

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/message.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>

static boolean
good_region(const region * r)
{
  return (!fval(r, RF_CHAOTIC) && r->age>30 && rplane(r)==NULL && r->units!=NULL && r->land!=NULL);
}

static int
cmp_age(const void * v1, const void *v2)
{
  const region * r1 = (const region*)v1;
  const region * r2 = (const region*)v2;
  if (r1->age<r2->age) return -1;
  if (r1->age>r2->age) return 1;
  return 0;
}

static building_type bt_wormhole = {
  "wormhole", BTF_NOBUILD|BTF_UNIQUE|BTF_INDESTRUCTIBLE,
  1, 4, 4,
  0, 0, 0, 0.0
};

typedef struct wormhole_data {
  building * entry;
  building * exit;
} wormhole_data;

static void 
wormhole_init(struct attrib *a)
{
  a->data.v = calloc(1, sizeof(wormhole_data));
}

static void 
wormhole_done(struct attrib * a)
{
  free(a->data.v);
}

static int
wormhole_age(struct attrib * a)
{
  wormhole_data * data = (wormhole_data*)a->data.v;
  int maxtransport = data->entry->size;
  region * r = data->entry->region;
  unit * u = r->units;

  for (;u!=NULL && maxtransport!=0;u=u->next) {
    message * m;
    if (u->number>maxtransport) continue;
    if (teure_talente(u)) continue;
    if (u->building!=data->entry) continue;

    move_unit(u, data->exit->region, NULL);
    maxtransport -= u->number;
    m = msg_message("wormhole_exit", "unit region", u, data->exit->region);
    add_message(&data->exit->region->msgs, m);
    add_message(&u->faction->msgs, m);
    msg_release(m);
  }

  /* it's important that destroy_building doesn't change b->region, because
   * otherwise the tunnel would no longer be bi-directional after this */
  destroy_building(data->entry);
  ADDMSG(&r->msgs, msg_message("wormhole_dissolve", "region", r));
  assert(data->entry->region==r);

  /* age returns 0 if the attribute needs to be removed, !=0 otherwise */
  return -1;
}

void 
wormhole_write(const struct attrib * a, FILE* F)
{
  wormhole_data * data = (wormhole_data*)a->data.v;
  write_building_reference(data->entry, F);
  write_building_reference(data->exit, F);
}

int
wormhole_read(struct attrib * a, FILE* F)
{
  wormhole_data * data = (wormhole_data*)a->data.v;

  read_building_reference(&data->entry, F);
  read_building_reference(&data->exit, F);
  /* return AT_READ_OK on success, AT_READ_FAIL if attrib needs removal */
  return AT_READ_OK;
}

static attrib_type at_wormhole = {
  "wormhole", 
  wormhole_init, 
  wormhole_done, 
  wormhole_age, 
  wormhole_write, 
  wormhole_read,
  ATF_UNIQUE
};

static void
make_wormhole(region * r1, region * r2)
{
  building * b1 = new_building(&bt_wormhole, r1, NULL);
  building * b2 = new_building(&bt_wormhole, r2, NULL);
  attrib * a1 = a_add(&b1->attribs, a_new(&at_wormhole));
  attrib * a2 = a_add(&b2->attribs, a_new(&at_wormhole));
  wormhole_data * d1 = (wormhole_data*)a1->data.v;
  wormhole_data * d2 = (wormhole_data*)a2->data.v;
  d1->entry = d2->exit = b1;
  d2->entry = d1->exit = b2;
  b1->size = bt_wormhole.maxsize;
  b2->size = bt_wormhole.maxsize;
  ADDMSG(&r1->msgs, msg_message("wormhole_appear", "region", r1));
  ADDMSG(&r2->msgs, msg_message("wormhole_appear", "region", r2));
}

void 
create_wormholes(void)
{
#define WORMHOLE_CHANCE 10000
  region_list *rptr, * rlist = NULL;
  region * r = regions;
  int i = 0, count = 0;
  region ** match;

  /*
   * select a list of regions. we'll sort them by age later.
   */
  while (r!=NULL) {
    int next = rand() % (2*WORMHOLE_CHANCE);
    while (r!=NULL && (next!=0 || !good_region(r))) {
      if (good_region(r)) {
        --next;
      }
      r=r->next;
    }
    if (r==NULL) break;
    add_regionlist(&rlist, r);
    ++count;
    r=r->next;
  }

  match = (region**)malloc(sizeof(region*) * count);
  rptr = rlist;
  while (i!=count) {
    match[i++] = rptr->data;
    rptr = rptr->next;
  }
  qsort(match, count, sizeof(region *), cmp_age);
  free_regionlist(rlist);

  count /= 2;
  for (i=0;i!=count;++i) {
    make_wormhole(match[i], match[i+count]);
  }
}

void 
register_wormholes(void)
{
  bt_register(&bt_wormhole);
  at_register(&at_wormhole);
}
#endif
