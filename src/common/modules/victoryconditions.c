/* vi: set ts=2:
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <eressea.h>
#include "victoryconditions.h"

/* kernel includes */
#include <region.h>
#include <faction.h>

/* util includes */
#include <attrib.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

#if VICTORY_CONDITION == VICTORY_MURDER

static void
print_winners_murder(const int winners)
{
	faction *f;
	char winner_buf[DISPLAYSIZE+1];
	message *msg;
	boolean first;

	winner_buf[0] = 0;

	first = true;
	for(f=factions; f; f=f->next) {
		if(f->no != 0 && f->victory_delay == VICTORY_DELAY) {
			if(first == true) {
				/* potential buffer overflow here */
				strcat(winner_buf, factionid(f));
				first = false;
			} else {
				/* potential buffer overflow here */
				strcat(winner_buf, ", ");
				strcat(winner_buf, factionid(f));
			}
		}
	}

	msg = msg_message("victory_murder_complete",
		"winners n", strdup(winner_buf), winners);

	for(f=factions; f; f=f->next) {
		add_message(&f->messages, msg);
	}
	msg_release(msg);
}

static boolean
is_winner_murder(const faction *f)
{
}

static void
check_victory_murder(void)
{
	faction *f;
	int condfulfilled = 0;
	int winners = 0;

	for(f=factions; f; f=f->next) {
		if(f->no != 0 && is_winner_murder(f)) {
			f->victory_delay++;
			condfulfilled++;
			if(f->victory_delay == VICTORY_DELAY)
				winners++;
		} else {
			f->victory_delay = 0;
		}
	}

	if(winners > 0) {
		print_winners_murder();
		return true;
	} else if(condfulfilled > 0) {
		for(f=factions; f; f=f->next) {
			if(f->victory_delay > 0) {
				faction *f2;
				message *msg = msg_message("victory_murder_cfulfilled",
					"faction remain", f, VICTORY_DELAY - f->victory_delay);
				for(f2=factions; f2; f2=f2->next) {
					add_message(&f2->messages, msg);
				}
				msg_release(msg);
			}
		}
	}
	return false;
}
#endif

void
check_victory(void)
{
#if VICTORY_CONDITION == VICTORY_MURDER
	check_victory_murder();
#endif
}

