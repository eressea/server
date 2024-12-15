#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "bind_eressea.h"

#include "eressea.h"
#include "json.h"
#include "orderfile.h"

#include <kernel/faction.h>
#include <kernel/item.h>
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
    if (filename) {
        return readgame(filename);
    }
    return -1;
}

int eressea_write_game(const char * filename) {
    if (filename) {
        remove_empty_factions();
        return writegame(filename);
    }
    return -1;
}

int eressea_read_orders(const char * filename) {
    return readorders(filename);
}

int eressea_export_json(const char * filename, int flags) {
    if (filename) {
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
    }
    return -1;
}

int eressea_import_json(const char * filename) {
    if (filename) {
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
    }
    return -1;
}
