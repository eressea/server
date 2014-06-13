#include "bind_config.h"

#include <platform.h>
#include <kernel/types.h>
#include <kernel/jsonconf.h>
#include <util/log.h>
#include <cJSON.h>

void config_parse(const char *json)
{
    cJSON * conf = cJSON_Parse(json);
    if (conf) {
        json_config(conf); 
        cJSON_Delete(conf);
    } else {
        log_error("json parse error: %s\n", cJSON_GetErrorPtr());
    }
}

void config_read(const char *filename)
{
}
