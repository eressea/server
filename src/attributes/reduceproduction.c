#include <platform.h>
#include <kernel/config.h>
#include "reduceproduction.h"
#include <kernel/region.h>
#include <kernel/messages.h>
#include <util/message.h>
#include <kernel/attrib.h>
#include <assert.h>

static int age_reduceproduction(attrib * a, void *owner)
{
    region * r = (region *)owner;
    int reduce = 100 - (5 * --a->data.sa[1]);
    assert(r);
    if (reduce < 10) {
        reduce = 10;
    }
    a->data.sa[0] = (short)reduce;
    if (a->data.sa[1] > 0) {
        ADDMSG(&r->msgs, msg_message("reduced_production", ""));
        return AT_AGE_KEEP;
    }
    return AT_AGE_REMOVE;
}

attrib_type at_reduceproduction = {
    "reduceproduction",
    NULL,
    NULL,
    age_reduceproduction,
    a_writeshorts,
    a_readshorts,
    NULL,
    ATF_UNIQUE
};

attrib *make_reduceproduction(int percent, int time)
{
    attrib *a = a_new(&at_reduceproduction);
    a->data.sa[0] = (short)percent;
    a->data.sa[1] = (short)time;
    return a;
}
