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

#ifndef H_KRNL_STUDY
#define H_KRNL_STUDY

#include "skill.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern int teach_cmd(struct unit *u, struct order *ord);
    extern int learn_cmd(struct unit *u, struct order *ord);

    extern magic_t getmagicskill(const struct locale *lang);
    extern bool is_migrant(struct unit *u);
    extern int study_cost(struct unit *u, skill_t talent);

#define MAXTEACHERS 4
    typedef struct teaching_info {
        struct unit *teachers[MAXTEACHERS];
        int value;
    } teaching_info;

    extern const struct attrib_type at_learning;

#ifdef __cplusplus
}
#endif
#endif
