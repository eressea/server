#include "bind_process.h"

#include <platform.h>
#include <kernel/types.h>
#include <kernel/order.h>
#include <kernel/battle.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <kernel/move.h>
#include <gamecode/economy.h>
#include <gamecode/laws.h>
#include <gamecode/market.h>
#include <gamecode/study.h>

#define PROC_LAND_REGION 0x0001
#define PROC_LONG_ORDER 0x0002

static void process_cmd(keyword_t kwd, int (*callback)(unit *, order *), int flags)
{
  region * r;
  for (r=regions; r; r=r->next) {
    unit * u;

    /* look for shortcuts */
    if (flags&PROC_LAND_REGION) {
      /* only execute when we are on solid terrain */
      while (r && (r->terrain->flags&LAND_REGION)==0) {
        r = r->next;
      }
      if (!r) break;
    }

    for (u=r->units; u; u=u->next) {
      if (flags & PROC_LONG_ORDER) {
        if (kwd == get_keyword(u->thisorder)) {
          callback(u, u->thisorder);
        }
      } else {
        order * ord;
        for (ord=u->orders; ord; ord=ord->next) {
          if (kwd == get_keyword(ord)) {
            callback(u, ord);
          }
        }
      }
    }
  }
}

void process_produce(void) {
  struct region *r;
  for (r = regions; r; r = r->next) {
    unit * u;
    for (u=r->units; u; u=u->next) {
      order * ord;
      for (ord=u->orders; ord; ord=ord->next) {
        if (K_MAKE == get_keyword(ord)) {
          make_cmd(u, ord);
        }
      }
    }
    produce(r);
    split_allocations(r);
  }
}

void process_battle(void) {
  struct region *r;
  for (r = regions; r; r = r->next) {
    do_battle(r);
  }
}

void process_siege(void) {
  process_cmd(K_BESIEGE, siege_cmd, PROC_LAND_REGION);
}

void process_update_long_order(void) {
  region * r;
  for (r=regions; r; r=r->next) {
    unit * u;
    for (u=r->units; u; u=u->next) {
      update_long_order(u);
    }
  }
}

void process_markets(void) {
  do_markets();
}

void process_make_temp(void) {
  new_units();
}

void process_settings(void) {
  region * r;
  for (r=regions; r; r=r->next) {
    unit * u;
    for (u=r->units; u; u=u->next) {
      order * ord;
      for (ord=u->orders; ord; ord=ord->next) {
        keyword_t kwd = get_keyword(ord);
        if (kwd==K_BANNER) {
          banner_cmd(u, ord);
        }
        else if (kwd==K_EMAIL) {
          email_cmd(u, ord);
        }
        else if (kwd==K_SEND) {
          send_cmd(u, ord);
        }
        else if (kwd==K_PASSWORD) {
          password_cmd(u, ord);
        }
      }
    }
  }
}

void process_ally(void) {
  process_cmd(K_ALLY, ally_cmd, 0);
}

void process_prefix(void) {
  process_cmd(K_PREFIX, prefix_cmd, 0);
}

void process_setstealth(void) {
  process_cmd(K_SETSTEALTH, setstealth_cmd, 0);
}

void process_status(void) {
  process_cmd(K_STATUS, status_cmd, 0);
}

void process_name(void) {
  process_cmd(K_NAME, name_cmd, 0);
  process_cmd(K_DISPLAY, display_cmd, 0);
}

void process_group(void) {
  process_cmd(K_GROUP, group_cmd, 0);
}

void process_origin(void) {
  process_cmd(K_URSPRUNG, origin_cmd, 0);
}

void process_quit(void) {
  process_cmd(K_QUIT, quit_cmd, 0);
  quit();
}

void process_study(void) {
  process_cmd(K_TEACH, teach_cmd, PROC_LONG_ORDER);
  process_cmd(K_STUDY, learn_cmd, PROC_LONG_ORDER);
}

void process_movement(void) {
  region * r;

  movement();
  for (r=regions; r; r=r->next) {
    if (r->ships) {
      sinkships(r);
    }
  }
}

void process_use(void) {
  process_cmd(K_USE, use_cmd, 0);
}

void process_leave(void) {
  process_cmd(K_LEAVE, leave_cmd, 0);
}

void process_promote(void) {
  process_cmd(K_PROMOTION, promotion_cmd, 0);
}

void process_renumber(void) {
  process_cmd(K_NUMBER, renumber_cmd, 0);
  renumber_factions();
}

void process_restack(void) {
  restack_units();
}

void process_maintenance(void) {
  region * r;
  for (r=regions; r; r=r->next) {
    unit * u;
    for (u=r->units; u; u=u->next) {
      order * ord;
      for (ord=u->orders; ord; ord=ord->next) {
        keyword_t kwd = get_keyword(ord);
        if (kwd==K_PAY) {
          pay_cmd(u, ord);
        }
      }
    }
    maintain_buildings(r, 0);
  }
}
