#include <kernel/types.h>
#include "donations.h"

#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/messages.h>

#include <stdlib.h>

void add_donation(faction * f1, faction * f2, int amount, region * r)
{
    donation *sp;

    sp = r->donations_;

    while (sp) {
        if (sp->f1 == f1 && sp->f2 == f2) {
            sp->amount += amount;
            return;
        }
        sp = sp->next;
    }

    sp = calloc(1, sizeof(donation));
    sp->f1 = f1;
    sp->f2 = f2;
    sp->amount = amount;
    sp->next = r->donations_;
    r->donations_ = sp;
}

void free_donations(void) {
    region *r;
    for (r = regions; r; r = r->next) {
        while (r->donations_) {
            donation *don = r->donations_;
            r->donations_ = don->next;
            free(don);
        }
    }
}

void report_donations(void)
{
    region *r;
    for (r = regions; r; r = r->next) {
        while (r->donations_) {
            donation *sp = r->donations_;
            if (sp->amount > 0) {
                struct message *msg = msg_message("donation",
                    "from to amount", sp->f1, sp->f2, sp->amount);
                r_addmessage(r, sp->f1, msg);
                r_addmessage(r, sp->f2, msg);
                msg_release(msg);
            }
            r->donations_ = sp->next;
            free(sp);
        }
    }
}
