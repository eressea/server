#include "bind_eressea.h"

#include <platform.h>
#include <kernel/types.h>
#include <kernel/config.h>
#include <kernel/save.h>
#include <util/storage.h>

void eressea_free_game(void) {
  free_gamedata();
}

int eressea_read_game(const char * filename) {
  return readgame(filename, IO_BINARY, 0);
} 

int eressea_write_game(const char * filename) {
  return writegame(filename, IO_BINARY);
}
