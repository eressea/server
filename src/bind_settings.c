#include "bind_settings.h"

#include <platform.h>
#include <kernel/config.h>

const char * settings_get(const char *key)
{
    return get_param(global.parameters, key);
}

void settings_set(const char *key, const char *value)
{
    set_param(&global.parameters, key, value);
}
