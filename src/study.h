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
#include <kernel/types.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct selist;

#define STUDYDAYS 30
#define TEACHNUMBER 10
#define TEACHDIFFERENCE 2

    typedef struct teaching_info {
        struct selist *teachers;
        int students;
        int days;
    } teaching_info;

    extern const struct attrib_type at_learning;

    int teach_cmd(struct unit *u, struct order *ord);
    int study_cmd(struct unit *u, struct order *ord);

    magic_t getmagicskill(const struct locale *lang);
    skill_t getskill(const struct locale *lang);
    bool is_migrant(struct unit *u);
    int study_cost(struct unit *u, skill_t sk);

    typedef void(*learn_fun)(struct unit *u, skill_t sk, int days);

    void learn_skill(struct unit *u, skill_t sk, int days);
    void reduce_skill_days(struct unit *u, skill_t sk, int days);

    void produceexp(struct unit *u, skill_t sk, int n);
    void produceexp_ex(struct unit *u, skill_t sk, int n, learn_fun learn);

    void demon_skillchange(struct unit *u);

    void inject_learn(learn_fun fun);

#ifdef __cplusplus
}
#endif
#endif
