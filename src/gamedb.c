#ifdef _MSC_VER
#include <platform.h>
#endif

#include "gamedb.h"

#include "kernel/database.h"
#include "kernel/faction.h"

void gamedb_update(void)
{
    faction *f;

    for (f = factions; f; f = f->next) {
        int uid = dblib_save_faction(f, turn);
        if (uid > 0) {
            f->uid = uid;
        }
    }
}
