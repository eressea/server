#ifdef _MSC_VER
#include <platform.h>
#endif

#include "contact.h"

#include "kernel/attrib.h"
#include "kernel/faction.h"
#include "kernel/order.h"
#include "kernel/unit.h"

static attrib_type at_contact_unit = {
    "contact_unit",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

static attrib_type at_contact_faction = {
    "contact_faction",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

void usetcontact(unit * u, const unit * u2)
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
    unit *u2;
    int n;

    init_order(ord, u->faction->locale);
    n = read_unitid(u->faction, u->region);
    u2 = findunit(n);

    if (u2 != NULL) {
        usetcontact(u, u2);
    }
    return 0;
}

