#include <platform.h>
#include <kernel/config.h>
#include "overrideroads.h"

#include <kernel/attrib.h>

attrib_type at_overrideroads = {
    "roads_override", NULL, NULL, NULL, a_writestring, a_readstring
};
