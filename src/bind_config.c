#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "bind_config.h"

#include "eressea.h"
#include "jsonconf.h"
#include "magic.h"

#include <kernel/config.h>
#include <kernel/building.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/ship.h>
#include <kernel/spell.h>

#include <util/language.h>
#include <util/log.h>
#include <util/nrmessage.h>
#include <util/path.h>
#include <util/strings.h>

#include <cJSON.h>

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void config_reset(void) {
    free_configuration();
}

int config_parse(const char *json)
{
    cJSON * conf = cJSON_Parse(json);
    if (conf) {
        reset_locales();
        json_config(conf);
        cJSON_Delete(conf);
        init_resources();
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
        str_strlcpy(buffer, xp, sizeof(buffer));
        buffer[9] = 0;
        log_error("json parse error in line %d, position %d, near `%s`\n", line, ep - lp, buffer);
    }
    return 1;
}

int config_read(const char *filename, const char * relpath)
{
    FILE *F;

    json_relpath = relpath;
    if (relpath) {
        char name[PATH_MAX];
        path_join(relpath, filename, name, sizeof(name));
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

