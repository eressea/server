/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <platform.h>
#include <kernel/config.h>
#include "option.h"

#include <kernel/save.h>
#include <util/attrib.h>

attrib_type at_option_news = {
	"option_news",
	NULL,
	NULL,
	NULL,
	a_writeint,
	a_readint,
	ATF_UNIQUE
};

void
init_option(void)
{
	at_register(&at_option_news);
}
