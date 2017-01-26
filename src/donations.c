#include <kernel/types.h>
#include "donations.h"

#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/messages.h>
#include <selist.h>

#include <stdlib.h>
#include <string.h>

typedef struct transfer {
    struct region *r;
    struct faction *f1, *f2;
    int amount;
} transfer;

static selist *transfers = 0;

int cmp_transfer(const void *v1, const void *v2) {
    const transfer *t1 = (const transfer *)v1;
    const transfer *t2 = (const transfer *)v2;
    if (t1->r == t2->r) {
        if (t1->f1 == t2->f1) {
            if (t1->f2 == t2->f2) {
                return 0;
            }
            return t1->f2->no - t2->f2->no;
        }
        return t1->f1->no - t2->f1->no;
    }
    return t1->r->uid - t2->r->uid;
}

void add_donation(faction * f1, faction * f2, int amount, region * r)
{
    transfer tr, *tf;
    selist *ql = transfers;
    int qi = 0;

    tr.r = r;
    tr.f1 = f1;
    tr.f2 = f2;
    tr.amount = amount;
    if (selist_set_find(&ql, &qi, &tr, cmp_transfer)) {
        tf = (transfer *)selist_get(ql, qi);
        tf->amount += amount;
    }
    else {
        tf = malloc(sizeof(transfer));
        memcpy(tf, &tr, sizeof(transfer));
    }
    selist_set_insert(&transfers, tf, cmp_transfer);
}

void free_donations(void) {
    selist_foreach(transfers, free);
    selist_free(transfers);
    transfers = 0;
}

static void report_transfer(void *data) {
    transfer *tf = (transfer *)data;
    if (tf->amount > 0) {
        struct message *msg = msg_message("donation",
             "from to amount", tf->f1, tf->f2, tf->amount);
        r_addmessage(tf->r, tf->f1, msg);
        r_addmessage(tf->r, tf->f2, msg);
        msg_release(msg);
    }
}

void report_donations(void)
{
    selist_foreach(transfers, report_transfer);
}
