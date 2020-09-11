#include "platform.h"
#include "kernel/config.h"
#include <kernel/unit.h>
#include <kernel/building.h>
#include <kernel/item.h>
#include <kernel/pool.h>

#include "academy.h"
#include "study.h"

void academy_teaching_bonus(struct unit *u, skill_t sk, int students) {
    if (students > 0 && sk != NOSKILL) {
        /* actually students * EXPERIENCEDAYS / MAX_STUDENTS */
        learn_skill(u, sk, students);
    }
}

bool academy_can_teach(unit *teacher, unit *scholar, skill_t sk) {
    const struct building_type *btype = bt_find("academy");
    return (active_building(scholar, btype)); 
}

