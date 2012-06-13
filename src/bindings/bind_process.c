#include "bind_process.h"

#include <platform.h>
#include <kernel/types.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/move.h>
#include <gamecode/economy.h>
#include <gamecode/laws.h>
#include <gamecode/market.h>
#include <gamecode/study.h>

static void process_cmd(keyword_t kwd, int (*callback)(unit *, order *))
{
  region * r;
  for (r=regions; r; r=r->next) {
    unit * u;
    for (u=r->units; u; u=u->next) {
      order * ord;
      for (ord=u->orders; ord; ord=ord->next) {
        if (kwd == get_keyword(ord)) {
          callback(u, ord);
        }
      }
    }
  }
}

static void process_long_cmd(keyword_t kwd, int (*callback)(unit *, order *))
{
  region * r;
  for (r=regions; r; r=r->next) {
    unit * u;
    for (u=r->units; u; u=u->next) {
      order * ord = u->thisorder;
      if (kwd == get_keyword(ord)) {
        callback(u, ord);
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
  process_cmd(K_ALLY, ally_cmd);
}

void process_prefix(void) {
  process_cmd(K_PREFIX, prefix_cmd);
}

void process_setstealth(void) {
  process_cmd(K_SETSTEALTH, setstealth_cmd);
}

void process_status(void) {
  process_cmd(K_STATUS, status_cmd);
}

void process_display(void) {
  process_cmd(K_DISPLAY, display_cmd);
}

void process_group(void) {
  process_cmd(K_GROUP, group_cmd);
}

void process_origin(void) {
  process_cmd(K_URSPRUNG, origin_cmd);
}

void process_quit(void) {
  process_cmd(K_QUIT, quit_cmd);
  quit();
}

void process_study(void) {
  process_long_cmd(K_TEACH, teach_cmd);
  process_long_cmd(K_STUDY, learn_cmd);
}

void process_movement(void) {
  movement();
}

void process_use(void) {
  process_cmd(K_USE, use_cmd);
}
