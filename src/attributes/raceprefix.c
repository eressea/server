#include <kernel/config.h>
#include "raceprefix.h"

#include <kernel/attrib.h>
#include <util/strings.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

attrib_type at_raceprefix = {
    "raceprefix", NULL, a_finalizestring, NULL, a_writestring, a_readstring, NULL,
    ATF_UNIQUE
};

void set_prefix(attrib ** ap, const char *str)
{
    attrib *a = a_find(*ap, &at_raceprefix);
    if (a == NULL) {
        a = a_add(ap, a_new(&at_raceprefix));
    }
    else {
        free(a->data.v);
    }
    assert(a->type == &at_raceprefix);
    a->data.v = str_strdup(str);
}

const char *get_prefix(attrib * a)
{
    char *str;
    a = a_find(a, &at_raceprefix);
    if (a == NULL)
        return NULL;
    str = (char *)a->data.v;
    /* conversion of old prefixes */
    if (strncmp(str, "prefix_", 7) == 0) {
        ((attrib *)a)->data.v = str_strdup(str + 7);
        free(str);
        str = (char *)a->data.v;
    }
    return str;
}
