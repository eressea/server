/* vi: set ts=2:
 *
 * Eressea PB(E)M host Copyright (C) 1998-2000
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
print_winners_murder(void)
{
	/* Eine oder mehrere Parteien haben endgültig gewonnen. Ausgeben. */
}

static boolean
is_winner_murder(const faction *f)
{
	/* Prüfen, ob Conditions erfüllt sind. */
	/* Wenn ja, counter erhöhen, sonst counter auf 0 */
	/* Wenn counter == VICTORY_DELAY: Partei hat gewonnen */
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
	} else if(condfulfilled > 0) {
		for(f=factions; f; f=f->next) {
			if(f->victory_delay > 0) {
				/* Meldung an alle, dass Partei x die Siegbedingung in der n-ten Woche
				 * erfüllt */
			}
		}
	}
}
#endif

void
check_victory(void)
{
#if VICTORY_CONDITION == VICTORY_MURDER
	check_victory_murder();
#endif
}

