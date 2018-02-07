#include <platform.h>
#include <kernel/curse.h>
#include <kernel/messages.h>
#include <util/language.h>

#include <stdlib.h>

static struct message *cinfo_magicresistance(const void *obj, objtype_t typ, const struct curse * c, int self)
{
    if (typ == TYP_UNIT) {
        if (self != 0) {
            const struct unit *u = (const struct unit *)obj;
            return msg_message("magicresistance_unit", "unit id", u,
                c->no);
        }
        return NULL;
    }
    if (typ == TYP_BUILDING) {
        const struct building *b = (const struct building *)obj;
        return msg_message("magicresistance_building", "id building", c->no, b);
    }
    return 0;
}

const struct curse_type ct_magicresistance = {
    "magicresistance", CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN, cinfo_magicresistance
};

void register_magicresistance(void)
{
    ct_register(&ct_magicresistance);
}
