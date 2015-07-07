#include <platform.h>
#include <kernel/config.h>
#include "unitcurse.h"
#include "buildingcurse.h"
#include <kernel/curse.h>

static struct message *cinfo_magicresistance(const void *obj, objtype_t typ, const struct curse * c, int self)
{
    if (typ == TYP_UNIT) {
        return cinfo_unit(obj, typ, c, self);
    }
    if (typ == TYP_BUILDING) {
        return cinfo_building(obj, typ, c, self);
    }
    return 0;
}

static struct curse_type ct_magicresistance = {
    "magicresistance", CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN, cinfo_magicresistance
};

void register_magicresistance(void)
{
    ct_register(&ct_magicresistance);
}
