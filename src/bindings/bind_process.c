#include "bind_process.h"

#include <platform.h>
#include <kernel/types.h>
#include <kernel/region.h>
#include <gamecode/economy.h>
#include <gamecode/market.h>

void process_produce(void) {
  struct region *r;
  for (r = regions; r; r = r->next) {
    produce(r);
  }
}

void process_markets(void) {
  do_markets();
}
