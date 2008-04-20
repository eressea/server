/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <kernel/eressea.h>
#include "shock.h"

/* kernel includes */
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/skill.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

/* util includes */
#include <util/base36.h>
#include <util/event.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/***
 ** shock
 **/

/* ------------------------------------------------------------- */
/* do_shock - Schockt die Einheit, z.B. bei Verlust eines */
/* Vertrauten.               */
/* ------------------------------------------------------------- */

static void
do_shock(unit *u, const char *reason)
{
  int i;

  if (u->number > 0) {
    /* HP - Verlust */
    u->hp = (unit_max_hp(u) * u->number)/10;
    u->hp = max(1, u->hp);
  }

  /* Aura - Verlust */
  if (is_mage(u)) {
    set_spellpoints(u, max_spellpoints(u->region,u)/10);
  }

  /* Evt. Talenttageverlust */
  for (i=0;i!=u->skill_size;++i) if (rng_int()%5==0) {
    skill * sv = u->skills+i;
    int weeks = (sv->level * sv->level - sv->level) / 2;
    int change = (weeks+9) / 10;
    reduce_skill(u, sv, change);
  }

  /* Dies ist ein Hack, um das skillmod und familiar-Attribut beim Mage
  * zu löschen wenn der Familiar getötet wird. Da sollten wir über eine
  * saubere Implementation nachdenken. */

  if (strcmp(reason, "trigger")==0) {
    remove_familiar(u);
  }
  if (u->faction!=NULL) {
    ADDMSG(&u->faction->msgs, msg_message("shock",
      "mage reason", u, strdup(reason)));
  }
}

static int
shock_handle(trigger * t, void * data)
{
	/* destroy the unit */
	unit * u = (unit*)t->data.v;
	if (u!=NULL) {
		do_shock(u, "trigger");
  } else {
    log_error(("could not perform shock::handle()\n"));
  }
	unused(data);
	return 0;
}

static void
shock_write(const trigger * t, FILE * F)
{
	unit * u = (unit*)t->data.v;
	trigger * next = t->next;
	while (next) {
		/* make sure it is unique! */
		if (next->type==t->type && next->data.v==t->data.v) break;
		next=next->next;
	}
	if (next && u) {
		log_error(("more than one shock-attribut for %s on a unit. FIXED.\n",
			unitid(u)));
		write_unit_reference(NULL, F);
	} else {
		write_unit_reference(u, F);
	}
}

static int
shock_read(trigger * t, FILE * F)
{
	return read_unit_reference((unit**)&t->data.v, F);
}

trigger_type tt_shock = {
	"shock",
	NULL,
	NULL,
	shock_handle,
	shock_write,
	shock_read
};

trigger *
trigger_shock(unit * u)
{
	trigger * t = t_new(&tt_shock);
	t->data.v = (void*)u;
	return t;
}
