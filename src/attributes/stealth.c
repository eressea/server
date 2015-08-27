#include <platform.h>
#include <kernel/unit.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <util/attrib.h>
#include <attributes/stealth.h>

#include <assert.h>
#include <stdlib.h>

attrib_type at_stealth = {
    "stealth", NULL, NULL, NULL, a_writeint, a_readint
};

void u_seteffstealth(unit * u, int value)
{
    if (skill_enabled(SK_STEALTH)) {
        attrib *a = NULL;
        if (u->flags & UFL_STEALTH) {
            a = a_find(u->attribs, &at_stealth);
        }
        if (value < 0) {
            if (a != NULL) {
                u->flags &= ~UFL_STEALTH;
                a_remove(&u->attribs, a);
            }
            return;
        }
        if (a == NULL) {
            a = a_add(&u->attribs, a_new(&at_stealth));
            u->flags |= UFL_STEALTH;
        }
        a->data.i = value;
    }
}

int u_geteffstealth(const unit *u)
{
    if (skill_enabled(SK_STEALTH)) {
        if (u->flags & UFL_STEALTH) {
            attrib *a = a_find(u->attribs, &at_stealth);
            if (a != NULL)
                return a->data.i;
        }
    }
    return -1;
}

/* r != u->region when called by cansee (see comment there) */
int eff_stealth(const unit * u, const region * r)
{
    int e = 0;

    /* Auf Schiffen keine Tarnung! */
    if (!u->ship && skill_enabled(SK_STEALTH)) {
        e = effskill(u, SK_STEALTH, r);

        if (u->flags & UFL_STEALTH) {
            int es = u_geteffstealth(u);
            if (es >= 0 && es < e)
                return es;
        }
    }
    return e;
}
