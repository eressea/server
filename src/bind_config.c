#include "bind_config.h"

#include <platform.h>
#include <kernel/types.h>
#include <kernel/jsonconf.h>
#include <cJSON.h>

void config_parse(const char *json)
{
    cJSON * conf = cJSON_Parse(json);
    if (conf) {
        json_config(conf); 
        cJSON_Delete(conf);
    }
}

void config_read(const char *filename)
{
}
