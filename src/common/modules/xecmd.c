/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>

#ifdef XECMD_MODULE
#include "xecmd.h"

#include <items/xerewards.h>

#include "xecmd.h"

/* kernel includes */
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/message.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/parser.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

attrib_type at_xontormiaexpress = {
	"xontormiaexpress",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	a_writeint,
	a_readint,
	ATF_UNIQUE
};

static void
xe_givelaen(unit *u, struct order * ord)
{
	unit *u2 =getunitg(u->region, u->faction);

	if(!u2) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
		return;
	}
	i_change(&u2->items, olditemtype[I_LAEN], 5);
}

static void
xe_givepotion(unit *u, struct order *ord)
{
	unit *u2 =getunitg(u->region, u->faction);

	if(!u2) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
		return;
	}
	i_change(&u2->items, it_find("skillpotion"), 1);
}

static void
xe_giveballon(unit *u, struct order *ord)
{
	unit *u2 =getunitg(u->region, u->faction);
	ship *sh;

	if(!u2) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
		return;
	}

	sh = new_ship(st_find("balloon"), u2->faction->locale, u2->region);
	sh->size = 5;
	set_string(&sh->name,"Xontormia-Ballon");
	leave(u2->region, u2);
	u2->ship = sh;
	fset(u2, UFL_OWNER);
}

int
xecmd(unit * u, order * ord)
{
  faction *f = u->faction;

  if (a_find(f->attribs, &at_xontormiaexpress)) {
    if (get_keyword(ord) == K_XE) {
      init_tokens(ord);
      skip_token();
      switch(findparam(getstrtoken(),f->locale)) {
        case P_XEPOTION:
          xe_givepotion(u, ord);
          break;
        case P_XEBALLOON:
          xe_giveballon(u, ord);
          break;
        case P_XELAEN:
          xe_givelaen(u, ord);
          break;
      }
    }
  }
  return 0;
}

#endif
