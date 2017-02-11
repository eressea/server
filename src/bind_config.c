#include "bind_config.h"

#include <platform.h>
#include <kernel/config.h>
#include <kernel/jsonconf.h>
#include <util/bsdstring.h>
#include <util/nrmessage.h>
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
    free_config();
    free_nrmesssages();
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
        strlcpy(buffer, xp, sizeof(buffer));
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
        join_path(relpath, filename, name, sizeof(name));
        F = fopen(name, "r");
    }
    else {
        F = fopen(filename, "r");
    }
    if (F) {
        long size;

        fseek(F, 0, SEEK_END);
        size = ftell(F);
        rewind(F);
        if (size > 0) {
            int result;
            char *data;
            size_t sz = (size_t)size;

            data = malloc(sz+1);
            sz = fread(data, 1, sz, F);
            data[sz] = 0;
            fclose(F);
            result = config_parse(data);
            free(data);
            return result;
        }
        fclose(F);
    }
    return 1;
}

