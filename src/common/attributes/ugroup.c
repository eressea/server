/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 */

#include <config.h>
#include "eressea.h"
#include "ugroup.h"

#include <attrib.h>

#ifdef USE_UGROUPS

attrib_type at_ugroup = {
	"ugroup", NULL, NULL, NULL, NULL, NULL, ATF_UNIQUE
};

void
init_ugroup(void)
{
	at_register(&at_ugroup);
}

#endif
