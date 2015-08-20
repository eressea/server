#include <platform.h>
#include <kernel/config.h>

#include "repair.h"
#include <kernel/faction.h>
#include <kernel/spellbook.h>

#include <util/log.h>
#include <util/base36.h>

#include <quicklist.h>

#include <stdio.h>
#include <stdlib.h>

static void write_spellbook_states(FILE *F) {
    faction *f;
    for (f = factions; f; f = f->next) {
        spellbook *sb = f->spellbook;
        int len = sb ? ql_length(sb->spells) : 0;
        fprintf(F, "%s %d %d\n", itoa36(f->no), f->subscription, len);
    }
}

static void limit_spellbook(faction *f, int num) {
    spellbook *sb = f->spellbook;
    int len = sb ? ql_length(sb->spells) : 0;
    if (len < num) {
        log_error("limit_spellbook: spellbook is shorter than expected, %d < %d", len, num);
    }
    // delete spells backwards from the end:
    while (len > num) {
        ql_delete(&sb->spells, len--);
    }
}

void repair_spells(const char *filename) {
    FILE *F = fopen(filename, "r");
    if (F) {
        char id[32];
        int numspells, sub;
        faction *f;
        while (fscanf(F, "%s %d %d", id, &sub, &numspells) != EOF) {
            int no = atoi36(id);

            f = findfaction(no);
            if (!f) {
                for (f = factions; f; f = f->next) {
                    if (f->subscription == sub) {
                        break;
                    }
                }
                if (f) {
                    log_info("repair_spells: faction %s renamed to %s, located by subscription %d", id, itoa36(f->no), sub);
                }
                else {
                    log_warning("repair_spells: cannot fix faction %s, no such subscription: %d (%d spells)", id, sub, numspells);
                    continue;
                }
            }
            if (f->subscription != sub) {
                log_warning("repair_spells: subscription mismatch for faction %s, %d!=%d (%d spells)", id, f->subscription, sub, numspells);
            }
            else {
                limit_spellbook(f, numspells);
                fset(f, FFL_MARK);
            }
        }
        for (f = factions; f; f = f->next) {
            if (!fval(f, FFL_MARK)) {
                numspells = ql_length(f->spellbook->spells);
                log_warning("repair_spells: faction %s did not get a spellbook fix (%d spells at level)", itoa36(f->no), numspells, f->max_spelllevel);
            }
            freset(f, FFL_MARK);
        }
    }
    else {
        F = fopen(filename, "w");
        if (!F) {
            perror("repair_spells");
            abort();
        }
        write_spellbook_states(F);
    }
    fclose(F);
}
