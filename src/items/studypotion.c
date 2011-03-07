#include <config.h>
#include <kernel/eressea.h>
#include "studypotion.h"

#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/skill.h>
#include <kernel/study.h>
#include <kernel/unit.h>

#include <util/attrib.h>
#include <util/functions.h>

/* BEGIN it_studypotion */
#define MAXGAIN 15
static int
use_studypotion(struct unit *u, const struct item_type *itype, int amount,
  struct order *ord)
{
  if (get_keyword(u->thisorder) == K_STUDY) {
    skill_t sk;
    skill *sv;

    init_tokens(u->thisorder);
    skip_token();
    sk = findskill(getstrtoken(), u->faction->locale);
    sv = get_skill(u, sk);

    if (sv && sv->level > 2) {
      /* TODO: message */
    } else if (study_cost(u, sk) > 0) {
      /* TODO: message */
    } else {
      attrib *a = a_find(u->attribs, &at_learning);
      teaching_info *teach;
      if (a == NULL) {
        a = a_add(&u->attribs, a_new(&at_learning));
      }
      teach = (teaching_info *) a->data.v;
      if (amount > MAXGAIN)
        amount = MAXGAIN;
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

void register_studypotion(void)
{
  register_function((pf_generic) use_studypotion, "use_studypotion");
}

/* END it_studypotion */
