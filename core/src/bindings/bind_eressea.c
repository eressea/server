#include "bind_eressea.h"

#include <platform.h>
#include <kernel/types.h>
#include <kernel/config.h>
#include <kernel/save.h>

#include <storage.h>

void eressea_free_game(void) {
  free_gamedata();
}

int eressea_read_game(const char * filename) {
  return readgame(filename, false);
} 

int eressea_write_game(const char * filename) {
  remove_empty_factions();
  return writegame(filename);
}

int eressea_read_orders(const char * filename) {
  return readorders(filename);
}
