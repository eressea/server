#ifdef _MSC_VER
#include <platform.h>
#endif

#include "gamedb.h"

#include "kernel/config.h"
#include "kernel/calendar.h"
#include "kernel/faction.h"
#include "kernel/db/driver.h"

#include "util/strings.h"

static int generate_factions(void *data, db_faction *results, int nresults)
{
    int i;
    faction **iter = (faction **)data;
    for (i = 0; *iter && i != nresults; ++i) {
        faction *f = *iter;
        const char *pwhash;
        results[i].p_uid = &f->uid;
        results[i].no = f->no;
        results[i].email = faction_getemail(f);
        pwhash = faction_getpassword(f);
        str_strlcpy(results[i].pwhash, pwhash ? pwhash : "", sizeof(results[i].pwhash));
        *iter = f->next;
    }
    return i;
}

int gamedb_update(void)
{
    int err;
    const char *dbname;

    dbname = config_get("game.dbname");

    err = db_driver_open(DB_GAME, dbname);
    if (err == 0) {
        faction *list = factions;
        db_driver_update_factions(generate_factions, &list);
        db_driver_compact(turn);
        db_driver_close(DB_GAME);
    }
    return err;
}
