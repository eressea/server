#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "export.h"
#include "magic.h"

#include "kernel/item.h"
#include "kernel/spell.h"
#include "kernel/spellbook.h"
#include "kernel/types.h"

#include "util/language.h"

#include <clibs/strings.h>

#include <stdbool.h>
#include <stdio.h>

typedef struct exporter {
    const struct locale *lang;
    FILE *F;
} exporter;

static int export_spell(spellbook_entry *sbe, void *udata) {
    exporter *exp = (exporter *)udata;
    spell *sp = spellref_get(&sbe->spref);
    const char *name = LOC(exp->lang, mkname("spell", sp->sname));
    const resource_type *rt_silver = get_resourcetype(R_SILVER);
    if (name) {
        int type = 0;
        int cost = 0;
        if (sp->sptyp & COMBATSPELL) type = 1;
        else if (sp->sptyp & POSTCOMBATSPELL) type = 2;
        else if (sp->sptyp & PRECOMBATSPELL) type = 3;
        if (sp->components) {
            for (int k = 0; sp->components[k].type; ++k) {
                const resource_type *rtype = sp->components[k].type;
                if (rtype == rt_silver) {
                    cost = sp->components[k].amount;
                    break;
                }
            }
        }
        fprintf(exp->F, "%s; %d; %d\n", name, cost, type);
    }
    return 0;
}

void export_echeck_spells(void)
{
    const struct locale *lang;
    for (lang = locales; lang; lang = nextlocale(lang)) {
        exporter exp;
        FILE *F;
        char filename[256];
        str_slprintf(filename, sizeof(filename), "spells-%s.txt", locale_name(lang));
        exp.lang = lang;
        if ((F = fopen(filename, "wt"))) {
            exp.F = F;
            for (magic_t m = (magic_t)1; m != MAXMAGIETYP; ++m) {
                spellbook *sb = get_spellbook(magic_school[m]);
                spellbook_foreach(sb, export_spell, &exp);
            }
            fclose(exp.F);
        }
    }
}

static int export_item(resource_type *rtype, void *udata) {
    exporter *exp = (exporter *)udata;
    const struct item_type *itype;
    if (NULL != (itype = rtype->itype)) {
        const char *s = locale_plural(exp->lang, rtype->_name, 1, false);
        const char *p = locale_plural(exp->lang, rtype->_name, 2, false);
        if (rtype->ltype) {
            int c = rtype->ltype->price;
            fprintf(exp->F, "%s; %s; %d\n", s, p, c);
        }
        else {
            fprintf(exp->F, "%s; %s\n", s, p);
        }
    }
    return 0;
}

void export_echeck_items(void)
{
    const struct locale *lang;
    for (lang = locales; lang; lang = nextlocale(lang)) {
        exporter exp;
        FILE *F;
        char filename[256];
        str_slprintf(filename, sizeof(filename), "items-%s.txt", locale_name(lang));
        exp.lang = lang;
        if ((F = fopen(filename, "wt"))) {
            exp.F = F;
            resource_foreach(export_item, &exp);
            fclose(exp.F);
        }
    }
}

