/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>
#include <kernel/unit.h>
#include <kernel/building.h>
#include <kernel/item.h>
#include <kernel/pool.h>
#include "academy.h"
#include "study.h"

void academy_teaching_bonus(struct unit *u, skill_t sk, int academy) {
    if (academy && sk != NOSKILL) {
        learn_skill(u, sk, academy / STUDYDAYS);
    }
}

bool academy_can_teach(unit *teacher, unit *student, skill_t sk) {
    const struct building_type *btype = bt_find("academy");
    if (active_building(teacher, btype) && active_building(student, btype)) {
        int j = study_cost(student, sk);
        j = _max(50, j * 2);
        /* kann Einheit das zahlen? */
        return get_pooled(student, get_resourcetype(R_SILVER), GET_DEFAULT, j) >= j;
        /* sonst nehmen sie nicht am Unterricht teil */
    }
    return false;
}
