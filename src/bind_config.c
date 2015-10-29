#include "bind_config.h"

#include <platform.h>
#include <kernel/jsonconf.h>
#include <util/log.h>
#include <util/language.h>
#include <cJSON.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "kernel/building.h"
#include "kernel/race.h"
#include "kernel/ship.h"
#include "kernel/spell.h"
#include "kernel/spellbook.h"
#include "kernel/terrain.h"

void config_reset(void) {
    default_locale = 0;
    free_locales();
    free_spells();
    free_buildingtypes();
    free_shiptypes();
    free_races();
    free_spellbooks();
}

int config_parse(const char *json)
{
    cJSON * conf = cJSON_Parse(json);
    if (conf) {
        json_config(conf);
        cJSON_Delete(conf);
        init_locales();
        return 0;
    }
    else {
        int line;
        char buffer[10];
        const char *xp = json, *lp, *ep = cJSON_GetErrorPtr();
        for (line = 1, lp = xp; xp && xp<ep; ++line, lp = xp + 1) {
            xp = strchr(lp, '\n');
            if (xp >= ep) break;
        }
        xp = (ep > json + 10) ? ep - 10 : json;
        strncpy(buffer, xp, sizeof(buffer));
        buffer[9] = 0;
        log_error("json parse error in line %d, position %d, near `%s`\n", line, ep - lp, buffer);
    }
    return 1;
}

int config_read(const char *filename, const char * relpath)
{
    char name[MAX_PATH];
    FILE *F;

    json_relpath = relpath;
    if (relpath) {
        _snprintf(name, sizeof(name), "%s/%s", relpath, filename);
        F = fopen(name, "rt");
    }
    else {
        F = fopen(filename, "rt");
    }
    if (F) {
        int result;
        char *data;

        fseek(F, 0, SEEK_END);
        result = ftell(F);
        rewind(F);
        if (result > 0) {
            size_t sz = (size_t)result;
            data = malloc(sz);
            fread(data, 1, sz, F);
            fclose(F);
        }
        result = config_parse(data);
        free(data);
        return result;
    }
    return 1;
}

