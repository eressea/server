/* vi: set ts=2:
 *
 * $Id: option.c,v 1.1 2001/03/01 07:05:15 corwin Exp $
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
#include "eressea.h"
#include "option.h"

#include <attrib.h>

attrib_type at_option_news = {
	"option_news",
	NULL,
	NULL,
	NULL,
	a_writedefault,
	a_readdefault,
	ATF_UNIQUE
};

void
init_option(void)
{
	at_register(&at_option_news);
}
