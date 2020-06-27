#ifdef _MSC_VER
#include <platform.h>
#endif
#include "bind_eressea.h"

#include "json.h"
#include "orderfile.h"

#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/config.h>
#include <kernel/save.h>

#include <util/language.h>
#include <util/log.h>

#include <stream.h>
#include <stdio.h>
#include <filestream.h>


void eressea_free_game(void) {
    free_gamedata();
    init_resources();
    init_locales(init_locale);
}

int eressea_read_game(const char * filename) {
    return readgame(filename);
}

int eressea_write_game(const char * filename) {
    remove_empty_factions();
    return writegame(filename);
}

int eressea_read_orders(const char * filename) {
    FILE * F = fopen(filename, "r");
    int result;

    if (!F) {
        perror(filename);
        return -1;
    }
    log_info("reading orders from %s", filename);
    result = parseorders(F);
    fclose(F);
    return result;
}

int eressea_export_json(const char * filename, int flags) {
    FILE *F = fopen(filename, "w");
    if (F) {
        stream out = { 0 };
        int err;
        fstream_init(&out, F);
        err = json_export(&out, flags);
        fstream_done(&out);
        return err;
    }
    perror(filename);
    return -1;
}

int eressea_import_json(const char * filename) {
    FILE *F = fopen(filename, "r");
    if (F) {
        stream out = { 0 };
        int err;
        fstream_init(&out, F);
        err = json_import(&out);
        fstream_done(&out);
        return err;
    }
    perror(filename);
    return -1;
}
