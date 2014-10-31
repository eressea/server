#include "types.h"
#include "ally.h"

#include <stdlib.h>

ally * ally_find(ally *al, const struct faction *f) {
    for (; al; al = al->next) {
        if (al->faction == f) return al;
    }
    return 0;
}

ally * ally_add(ally **al_p, struct faction *f) {
    ally * al;
    if (f) {
        while (*al_p) {
            al = *al_p;
            if (al->faction == f) return al;
            al_p = &al->next;
        }
    }
    al = (ally *)malloc(sizeof(ally));
    al->faction = f;
    al->status = 0;
    al->next = 0;
    *al_p = al;
    return al;
}

void ally_remove(ally **al_p, struct faction *f) {
    ally * al;
    while (*al_p) {
        al = *al_p;
        if (al->faction == f) {
            *al_p = al->next;
            free(al);
            break;
        }
        al_p = &al->next;
    }
}
