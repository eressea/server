/* vi: set ts=2:
 *
 * $Id: spells.c,v 1.3 2001/02/03 13:45:33 enno Exp $
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
#include "spells.h"

#include "alp.h"

/*
#include "firewall.h" 
*/
struct curse_type;
extern const struct curse_type ct_firewall;
extern void ct_register(const struct curse_type * ct);

void
init_spells(void)
{
	/* sp_summon_alp */
	init_alp();
	/* init_firewall(); */
	ct_register(&ct_firewall);
}
