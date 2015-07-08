#include <platform.h>
#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/messages.h>
#include <util/language.h>

static struct message *cinfo_magicresistance(const void *obj, objtype_t typ, const struct curse * c, int self)
{
    if (typ == TYP_UNIT) {
        if (self != 0) {
            const struct unit *u = (const struct unit *)obj;
            return msg_message(mkname("curseinfo", c->type->cname), "unit id", u,
                c->no);
        }
        return NULL;
    }
    if (typ == TYP_BUILDING) {
        const struct building *b = (const struct building *)obj;
        return msg_message(mkname("curseinfo", self ? "homestone" : "buildingunknown"), "id building", c->no, b);
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
