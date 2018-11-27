#ifdef _MSC_VER
#include <platform.h>
#endif

#include "contact.h"

#include "kernel/attrib.h"
#include "kernel/faction.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/unit.h"

#include "util/base36.h"
#include "util/param.h"
#include "util/parser.h"

#include <assert.h>

static attrib_type at_contact_unit = {
    "contact_unit",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

void contact_unit(unit * u, const unit * u2)
{
    attrib *a = a_find(u->attribs, &at_contact_unit);
    while (a && a->type == &at_contact_unit) {
        if (a->data.v == u2) {
            return;
        }
        a = a->next;
    }
    a_add(&u->attribs, a_new(&at_contact_unit))->data.v = (void *)u2;
}

static attrib_type at_contact_faction = {
    "contact_faction",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

void contact_faction(unit * u, const faction * f)
{
    attrib *a = a_find(u->attribs, &at_contact_faction);
    while (a && a->type == &at_contact_faction) {
        if (a->data.v == f) {
            return;
        }
        a = a->next;
    }
    a_add(&u->attribs, a_new(&at_contact_faction))->data.v = (void *)f;
}

bool ucontact(const unit * u, const unit * u2)
/* Prueft, ob u den Kontaktiere-Befehl zu u2 gesetzt hat. */
{
    attrib *a;

    if (u->faction == u2->faction) {
        return true;
    }
    /* Explizites KONTAKTIERE */
    for (a = u->attribs; a; a = a->next) {
        if (a->type == &at_contact_unit) {
            if (u2 == a->data.v) {
                return true;
            }
        }
        else if (a->type == &at_contact_faction) {
            if (u2->faction == a->data.v) {
                return true;
            }
        }
    }
    return false;
}

int contact_cmd(unit * u, order * ord)
{
    char token[16];
    const char *str;
    param_t p;

    init_order(ord, u->faction->locale);
    str = gettoken(token, sizeof(token));
    p = findparam(str, u->faction->locale);
    if (p == P_FACTION) {
        /* new-style syntax, KONTAKTIERE PARTEI foo */
        faction * f = getfaction();
        if (!f) {
            cmistake(u, ord, 66, MSG_EVENT);
        }
        else {
            contact_faction(u, f);
            return 0;
        }
    }
    else if (p == P_UNIT) {
        /* new-style syntax, KONTAKTIERE EINHEIT [TEMP] foo */
        unit *u2 = NULL;
        if (GET_UNIT == getunit(u->region, u->faction, &u2)) {
            assert(u2);
            contact_unit(u, u2);
            return 0;
        }
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord,
            "feedback_unit_not_found", NULL));
    }
    else {
        /* old-style syntax, KONTAKTIERE foo */
        unit *u2;
        int n = 0;
        if (p == P_TEMP) {
            n = getid();
            u2 = findnewunit(u->region, u->faction, n);
        }
        else {
            n = atoi36((const char *)str);
            u2 = findunit(n);
        }
        if (u2 != NULL) {
            contact_unit(u, u2);
            return 0;
        }
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord,
            "feedback_unit_not_found", NULL));
    }
    return -1;
}
