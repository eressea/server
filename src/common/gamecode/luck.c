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

#include "luck.h"

/* attributes includes */
#include <attributes/racename.h>
#include <items/weapons.h>

#include "economy.h"
#include "randenc.h"

/* kernel include */
#include <unit.h>
#include <faction.h>
#include <item.h>
#include <message.h>
#include <race.h>
#include <pool.h>
#include <region.h>
#include <skill.h>
#include <karma.h>

/* util includes */
#include <util/rand.h>
#include <util/rng.h>
#include <util/message.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#ifdef KARMA_MODULE

#define STANDARD_LUCK 0

static void
lucky_silver(const unit *u)
{
	int i = 0, r, max = 0;
	int luck = fspecial(u->faction, FS_LUCKY);

	do {
		r = 1 + rng_int()%(10000*(luck+STANDARD_LUCK));
		if(r > max) max = r;
		i++;
	} while(i <= luck);
	
	i_change(&((unit *)u)->items, i_silver, max);
	
  ADDMSG(&u->faction->msgs, msg_message("lucky_item",
    "unit item amount", u, i_silver->rtype, max));
}

typedef struct luckyitem {
	const char * name;
	int weight; /* weighting the item */
} luckyitem;

static void
lucky_item(const unit *u)
{
	int i = 0, r, max = 0;
	item_type *itype;
	int amount;
	int luck = fspecial(u->faction, FS_LUCKY);
	/* Das ist schöner so, denke ich... */
	static int nitems = 0;
	static int maxweight = 0;
	struct luckyitem it_list[] = {
		{ "mallorn", 1 },
		{ NULL, 0 }
	};
	if (nitems==0) {
		while (it_list[nitems].name) {
			maxweight +=it_list[nitems].weight;
			++nitems;
		}
	}
	/* weight is unused */
	r = rng_int()%nitems;

	do {
		r = rng_int()%nitems;
		if(r > max) max = r;
		i++;
	} while(i <= luck);

	itype = it_find(it_list[r].name);

	if(luck)
		amount = 10 + rng_int()%(luck*40) + rng_int()%(luck*40);
	else 
		amount = 5 + rng_int()%10 +rng_int()%10;

	i_change(&((unit *)u)->items, itype, amount);
  ADDMSG(&u->faction->msgs, msg_message("lucky_item", 
    "unit item amount", u, itype->rtype, amount));
}

static void
lucky_magic_item(const unit *u)
{
#if defined(__GNUC__) && !defined(__STDC__)
	item_type *itype;
	int amount;
	int luck = fspecial(u->faction, FS_LUCKY);
	const int n_items = 13;
	item_type *it_list[] = {
		olditemtype[I_AMULET_OF_HEALING],
		olditemtype[I_AMULET_OF_TRUE_SEEING],
		olditemtype[I_RING_OF_INVISIBILITY],
		olditemtype[I_RING_OF_POWER],
		olditemtype[I_FIRESWORD],
		olditemtype[I_CHASTITY_BELT],
		olditemtype[I_FEENSTIEFEL],
		olditemtype[I_ANTIMAGICCRYSTAL],
		olditemtype[I_RING_OF_NIMBLEFINGER],
		olditemtype[I_TROLLBELT],
		olditemtype[I_TACTICCRYSTAL],
		olditemtype[I_SACK_OF_CONSERVATION],
	};

	itype = it_list[rng_int()%n_items];
	amount = 1 + rng_int()%luck;

	i_change(&((unit *)u)->items, itype, amount);
	ADDMSG(&u->faction->msgs, msg_message("lucky_item", 
    "unit item amount", u, itype->rtype, amount));
#endif
}


static void
lucky_event(const faction *f)
{
	const unit *u = random_unit_in_faction(f);

	if(!u) return;

	switch(rng_int()%3) {
	case 0:
		lucky_silver(u);
		break;
	case 1:
		lucky_item(u);
		break;
	case 2:
		lucky_magic_item(u);
		break;
	}
}

void
check_luck(void)
{
	faction *f;

  for(f=factions; f; f=f->next) {
    if(rng_int()%100 < STANDARD_LUCK+fspecial(f, FS_LUCKY)*8)
			lucky_event(f);
	}
}
#endif /* KARMA_MODULE */
