#include <config.h>
#include <kernel/eressea.h>
#include "items.h"

#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/message.h>
#include <kernel/movement.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/study.h>
#include <kernel/unit.h>

#include <util/attrib.h>
#include <util/functions.h>

/* BEGIN studypotion */
#define MAXGAIN 15
static int
use_studypotion(struct unit * u, const struct item_type * itype, int amount, struct order * ord)
{
  if (get_keyword(u->thisorder) == K_STUDY) {
    skill_t sk;
    skill * sv;

    init_tokens(u->thisorder);
    skip_token();
    sk = findskill(getstrtoken(), u->faction->locale);
    sv = get_skill(u, sk);

    if (sv && sv->level > 2) {
      /* TODO: message */
    } else if (study_cost(u, sk)>0) {
      /* TODO: message */
    } else {
      attrib * a = a_find(u->attribs, &at_learning);
      teaching_info * teach;
      if (a==NULL) {
        a = a_add(&u->attribs, a_new(&at_learning));
      }
      teach = (teaching_info*) a->data.v;
      if (amount>MAXGAIN) amount = MAXGAIN;
      teach->value += amount * 30;
      if (teach->value > MAXGAIN * 30) {
        teach->value = MAXGAIN * 30;
      }
      i_change(&u->items, itype, -amount);
      return 0;
    }
  }
  return EUNUSABLE;
}
/* END studypotion */

/* BEGIN speedsail */
static int
use_speedsail(struct unit * u, const struct item_type * itype, int amount, struct order * ord)
{
  struct plane * p = rplane(u->region);
  unused(amount);
  unused(itype);
  if (p!=NULL) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "use_realworld_only", ""));
  } else {
    if (u->ship) {
      attrib * a = a_find(u->ship->attribs, &at_speedup);
      if (a==NULL) {
        a = a_add(&u->ship->attribs, a_new(&at_speedup));
        a->data.sa[0] = 50; /* speed */
        a->data.sa[1] = 50; /* decay */
        ADDMSG(&u->faction->msgs, msg_message("use_speedsail", "unit", u));
        /* Ticket abziehen */
        i_change(&u->items, itype, -1);
        return 0;
      } else {
        cmistake(u, ord, 211, MSG_EVENT);
      }
    } else {
      cmistake(u, ord, 144, MSG_EVENT);
    }
  }
  return EUNUSABLE;
}
/* END speedsail */

void
register_itemimplementations(void)
{
  register_function((pf_generic)use_studypotion, "use_studypotion");
	register_function((pf_generic)use_speedsail, "use_speedsail");
}
