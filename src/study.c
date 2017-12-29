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
#include "study.h"
#include "laws.h"
#include "move.h"
#include "monsters.h"
#include "alchemy.h"
#include "academy.h"

#include <spells/regioncurse.h>

#include <kernel/ally.h>
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/language.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/umlaut.h>

#include <selist.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TEACH_ALL 1
#define TEACH_FRIENDS

static skill_t getskill(const struct locale *lang)
{
    char token[128];
    const char * s = gettoken(token, sizeof(token));
    return s ? get_skill(s, lang) : NOSKILL;
}

magic_t getmagicskill(const struct locale * lang)
{
    void **tokens = get_translations(lang, UT_MAGIC);
    variant token;
    const char *s = getstrtoken();

    if (tokens && s && s[0]) {
        if (findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
            return (magic_t)token.i;
        }
        else {
            char buffer[8];
            buffer[0] = s[0];
            buffer[1] = s[1];
            buffer[2] = '\0';
            if (findtoken(*tokens, buffer, &token) == E_TOK_SUCCESS) {
                return (magic_t)token.i;
            }
        }
    }
    return M_NONE;
}

/* ------------------------------------------------------------- */
/* familiars and toads are not migrants */
bool is_migrant(unit * u)
{
    static int cache;
    static const race *toad_rc;

    if (u_race(u) == u->faction->race)
        return false;

    if (fval(u_race(u), RCF_UNDEAD | RCF_ILLUSIONARY))
        return false;
    if (is_familiar(u))
        return false;
    if (rc_changed(&cache)) {
        toad_rc = get_race(RC_TOAD);
    }
    return u_race(u) != toad_rc;
}

/* ------------------------------------------------------------- */
bool magic_lowskill(unit * u)
{
    static const race *toad_rc;
    static int cache;
    if (rc_changed(&cache)) {
        toad_rc = get_race(RC_TOAD);
    }
    return u_race(u) == toad_rc;
}

/* ------------------------------------------------------------- */

int study_cost(struct unit *u, skill_t sk)
{
    static int config;
    static int costs[MAXSKILLS];
    int cost = -1;

    if (sk == SK_MAGIC) {
        int next_level = 1 + (u ? get_level(u, sk) : 0);
        /* Die Magiekosten betragen 50+Summe(50*Stufe) */
        /* 'Stufe' ist dabei die naechste zu erreichende Stufe */
        cost = 50 * (1 + ((next_level + 1) * next_level / 2));
    }
    else switch (sk) {
    case SK_SPY:
        cost = 100;
        break;
    case SK_TACTICS:
    case SK_HERBALISM:
    case SK_ALCHEMY:
        cost = 200;
        break;
    default:
        cost = -1;
    }

    if (config_changed(&config)) {
        memset(costs, 0, sizeof(costs));
    }

    if (costs[sk] == 0) {
        char buffer[256];
        sprintf(buffer, "skills.cost.%s", skillnames[sk]);
        costs[sk] = config_get_int(buffer, cost);
    }
    if (costs[sk] >= 0) {
        return costs[sk];
    }
    return (cost > 0) ? cost : 0;
}

/* ------------------------------------------------------------- */

static void init_learning(struct attrib *a)
{
    a->data.v = calloc(sizeof(teaching_info), 1);
}

static void done_learning(struct attrib *a)
{
    teaching_info *teach = (teaching_info *)a->data.v;
    selist_free(teach->teachers);
    free(a->data.v);
}

const attrib_type at_learning = {
    "learning",
    init_learning, done_learning, NULL, NULL, NULL, NULL,
    ATF_UNIQUE
};

#define EXPERIENCEDAYS 10

static int study_days(unit * student, skill_t sk)
{
    int speed = STUDYDAYS;
    if (u_race(student)->study_speed) {
        speed += u_race(student)->study_speed[sk];
        if (speed < STUDYDAYS) {
            skill *sv = unit_skill(student, sk);
            if (sv == 0) {
                speed = STUDYDAYS;
            }
        }
    }
    return student->number * speed;
}

static int
teach_unit(unit * teacher, unit * student, int nteaching, skill_t sk,
    bool report, int *academy_students)
{
    teaching_info *teach = NULL;
    attrib *a;
    int students;

    if (magic_lowskill(student)) {
        cmistake(teacher, teacher->thisorder, 292, MSG_EVENT);
        return 0;
    }

    students = student->number;
    /* subtract already taught students */
    a = a_find(student->attribs, &at_learning);
    if (a != NULL) {
        teach = (teaching_info *)a->data.v;
        students -= teach->students;
    }

    if (students > nteaching) students = nteaching;

    if (students > 0) {
        if (teach == NULL) {
            a = a_add(&student->attribs, a_new(&at_learning));
            teach = (teaching_info *)a->data.v;
        }
        selist_push(&teach->teachers, teacher);
        teach->days += students * STUDYDAYS;
        teach->students += students; 

        if (student->building && teacher->building == student->building) {
            /* Solange Akademien groessenbeschraenkt sind, sollte Lehrer und
             * Student auch in unterschiedlichen Gebaeuden stehen duerfen */
            /* FIXME comment contradicts implementation */
            if (academy_can_teach(teacher, student, sk)) {
                /* Jeder Schueler zusaetzlich +10 Tage wenn in Uni. */
                teach->days += students * EXPERIENCEDAYS;  /* learning erhoehen */
                /* Lehrer zusaetzlich +1 Tag pro Schueler. */
                if (academy_students) {
                    *academy_students += students;
                }
            }
        }
    }
    return students;
}

int teach_cmd(unit * teacher, struct order *ord)
{
    plane *pl;
    region *r = teacher->region;
    skill_t sk_academy = NOSKILL;
    int teaching, i, j, count, academy_students = 0;

    if (r->attribs) {
        if (get_curse(r->attribs, &ct_gbdream)) {
            ADDMSG(&teacher->faction->msgs,
                msg_feedback(teacher, ord, "gbdream_noteach", ""));
            return 0;
        }
    }
    if ((u_race(teacher)->flags & RCF_NOTEACH) || fval(teacher, UFL_WERE)) {
        cmistake(teacher, ord, 274, MSG_EVENT);
        return 0;
    }

    pl = rplane(r);
    if (pl && fval(pl, PFL_NOTEACH)) {
        cmistake(teacher, ord, 273, MSG_EVENT);
        return 0;
    }

    teaching = teacher->number  * TEACHNUMBER;

    if ((i = get_effect(teacher, oldpotiontype[P_FOOL])) > 0) { /* Trank "Dumpfbackenbrot" */
        if (i > teaching) i = teaching;
        /* Trank wirkt pro Schueler, nicht pro Lehrer */
        teaching -= i;
        change_effect(teacher, oldpotiontype[P_FOOL], -i);
        j = teaching;
        ADDMSG(&teacher->faction->msgs, msg_message("teachdumb", "teacher amount", teacher, j));
    }
    if (teaching <= 0)
        return 0;

    count = 0;

    init_order_depr(ord);

#if TEACH_ALL
    if (getparam(teacher->faction->locale) == P_ANY) {
        skill_t sk;
        unit *student;
        skill_t teachskill[MAXSKILLS];
        int t = 0;

        do {
            sk = getskill(teacher->faction->locale);
            teachskill[t] = getskill(teacher->faction->locale);
        } while (sk != NOSKILL);

        for (student = r->units; teaching > 0 && student; student = student->next) {
            if (LongHunger(student)) {
                continue;
            }
            else if (student->faction == teacher->faction) {
                if (getkeyword(student->thisorder) == K_STUDY) {
                    /* Input ist nun von student->thisorder !! */
                    init_order(student->thisorder, student->faction->locale);
                    sk = getskill(student->faction->locale);
                    if (sk != NOSKILL && teachskill[0] != NOSKILL) {
                        for (t = 0; teachskill[t] != NOSKILL; ++t) {
                            if (sk == teachskill[t]) {
                                break;
                            }
                        }
                        sk = teachskill[t];
                    }
                    if (sk != NOSKILL
                        && effskill_study(teacher, sk, 0) - TEACHDIFFERENCE > effskill_study(student, sk, 0)) {
                        teaching -= teach_unit(teacher, student, teaching, sk, true, &academy_students);
                    }
                }
            }
#ifdef TEACH_FRIENDS
            else if (alliedunit(teacher, student->faction, HELP_GUARD)) {
                if (getkeyword(student->thisorder) == K_STUDY) {
                    /* Input ist nun von student->thisorder !! */
                    init_order(student->thisorder, student->faction->locale);
                    sk = getskill(student->faction->locale);
                    if (sk != NOSKILL
                        && effskill_study(teacher, sk, 0) - TEACHDIFFERENCE >= effskill(student, sk, 0)) {
                        teaching -= teach_unit(teacher, student, teaching, sk, true, &academy_students);
                    }
                }
            }
#endif
        }
    }
    else
#endif
    {
        char zOrder[4096];
        size_t sz = sizeof(zOrder);
        order *new_order;

        zOrder[0] = '\0';
        init_order_depr(ord);

        while (!parser_end()) {
            skill_t sk;
            unit *student;
            bool feedback;

            getunit(r, teacher->faction, &student);
            ++count;

            /* Falls die Unit nicht gefunden wird, Fehler melden */

            if (!student) {
                char tbuf[20];
                const char *uid;
                const char *token;
                /* Finde den string, der den Fehler verursacht hat */
                parser_pushstate();
                init_order_depr(ord);

                for (j = 0; j != count - 1; ++j) {
                    /* skip over the first 'count' units */
                    getunit(r, teacher->faction, NULL);
                }

                token = getstrtoken();

                /* Beginne die Fehlermeldung */
                if (isparam(token, teacher->faction->locale, P_TEMP)) {
                    token = getstrtoken();
                    sprintf(tbuf, "%s %s", LOC(teacher->faction->locale,
                        parameters[P_TEMP]), token);
                    uid = tbuf;
                }
                else {
                    uid = token;
                }
                ADDMSG(&teacher->faction->msgs, msg_feedback(teacher, ord, "unitnotfound_id",
                    "id", uid));

                parser_popstate();
                continue;
            }

            feedback = teacher->faction == student->faction
                || alliedunit(student, teacher->faction, HELP_GUARD);

            /* Neuen Befehl zusammenbauen. TEMP-Einheiten werden automatisch in
             * ihre neuen Nummern uebersetzt. */
            if (zOrder[0]) {
                strncat(zOrder, " ", sz - 1);
                --sz;
            }
            sz -= strlcpy(zOrder + 4096 - sz, itoa36(student->no), sz);

            if (getkeyword(student->thisorder) != K_STUDY) {
                ADDMSG(&teacher->faction->msgs,
                    msg_feedback(teacher, ord, "teach_nolearn", "student", student));
                continue;
            }

            /* Input ist nun von student->thisorder !! */
            parser_pushstate();
            init_order(student->thisorder, student->faction->locale);
            sk = getskill(student->faction->locale);
            parser_popstate();

            if (sk == NOSKILL) {
                ADDMSG(&teacher->faction->msgs,
                    msg_feedback(teacher, ord, "teach_nolearn", "student", student));
                continue;
            }

            if (effskill_study(student, sk, 0) > effskill_study(teacher, sk, 0)
                - TEACHDIFFERENCE) {
                if (feedback) {
                    ADDMSG(&teacher->faction->msgs, msg_feedback(teacher, ord, "teach_asgood",
                        "student", student));
                }
                continue;
            }
            if (sk == SK_MAGIC) {
                /* ist der Magier schon spezialisiert, so versteht er nur noch
                 * Lehrer seines Gebietes */
                sc_mage *mage1 = get_mage_depr(teacher);
                sc_mage *mage2 = get_mage_depr(student);
                if (mage2 && mage1 && mage2->magietyp != M_GRAY
                    && mage1->magietyp != mage2->magietyp) {
                    if (feedback) {
                        ADDMSG(&teacher->faction->msgs, msg_feedback(teacher, ord,
                            "error_different_magic", "target", student));
                    }
                    continue;
                }
            }
            sk_academy = sk;
            teaching -= teach_unit(teacher, student, teaching, sk, false, &academy_students);
        }
        new_order = create_order(K_TEACH, teacher->faction->locale, "%s", zOrder);
        replace_order(&teacher->orders, ord, new_order);
        free_order(new_order);      /* parse_order & set_order have each increased the refcount */
    }
    if (academy_students > 0 && sk_academy!=NOSKILL) {
        academy_teaching_bonus(teacher, sk_academy, academy_students);
    }
    init_order_depr(NULL);
    return 0;
}

typedef enum study_rule_t {
    STUDY_DEFAULT = 0,
    STUDY_FASTER = 1,
    STUDY_AUTOTEACH = 2
} study_rule_t;

static double study_speedup(unit * u, skill_t s, study_rule_t rule)
{
#define MINTURN 16
    double learnweeks = 0;
    int i;
    if (turn > MINTURN) {
        if (rule == STUDY_FASTER) {
            for (i = 0; i != u->skill_size; ++i) {
                skill *sv = u->skills + i;
                if (sv->id == s) {
                    learnweeks = sv->level * (sv->level + 1) / 2.0;
                    if (learnweeks < turn / 3.0) {
                        return 2.0;
                    }
                }
            }
            return 2.0; /* If the skill was not found it is the first study. */
        }
        if (rule == STUDY_AUTOTEACH) {
            for (i = 0; i != u->skill_size; ++i) {
                skill *sv = u->skills + i;
                learnweeks += (sv->level * (sv->level + 1) / 2.0);
            }
            if (learnweeks < turn / 2.0) {
                return 2.0;
            }
        }
    }
    return 1.0;
}

static bool ExpensiveMigrants(void)
{
	static bool rule;
	static int cache;
	if (config_changed(&cache)) {
		rule = config_get_int("study.expensivemigrants", 0) != 0;
	}
	return rule;
}

struct teach_data {
    unit *u;
    skill_t sk;
};

static bool cb_msg_teach(void *el, void *arg) {
    struct teach_data *td = (struct teach_data *)arg;
    unit *ut = (unit *)el;
    unit * u = td->u;
    skill_t sk = td->sk;
    if (ut->faction != u->faction) {
        bool feedback = alliedunit(u, ut->faction, HELP_GUARD);
        if (feedback) {
            ADDMSG(&ut->faction->msgs, msg_message("teach_teacher",
                "teacher student skill level", ut, u, sk,
                effskill(u, sk, 0)));
        }
        ADDMSG(&u->faction->msgs, msg_message("teach_student",
            "teacher student skill", ut, u, sk));
    }
    return true;
}

static void msg_teachers(struct selist *teachers, struct unit *u, skill_t sk) {
    struct teach_data cbdata;
    cbdata.sk = sk;
    cbdata.u = u;
    selist_foreach_ex(teachers, cb_msg_teach, &cbdata);
}

int study_cmd(unit * u, order * ord)
{
    region *r = u->region;
    int p;
    magic_t mtyp;
    int l;
    int studycost, days;
    double multi = 1.0;
    attrib *a = NULL;
    teaching_info *teach = NULL;
    int money = 0;
    skill_t sk;
    int maxalchemy = 0;
    int speed_rule = (study_rule_t)config_get_int("study.speedup", 0);
    bool learn_newskills = config_get_int("study.newskills", 1) != 0;
    static const race *rc_snotling;
    static int rc_cache;

    if (rc_changed(&rc_cache)) {
        rc_snotling = get_race(RC_SNOTLING);
    }

    if (!unit_can_study(u)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_race_nolearn", "race",
            u_race(u)));
        return -1;
    }

    (void)init_order(ord, u->faction->locale);
    sk = getskill(u->faction->locale);

    if (sk < 0) {
        cmistake(u, ord, 77, MSG_EVENT);
        return -1;
    }
    /* Hack: Talente mit Malus -99 koennen nicht gelernt werden */
    if (u_race(u)->bonus[sk] == -99) {
        cmistake(u, ord, 771, MSG_EVENT);
        return -1;
    }
    if (!learn_newskills) {
        skill *sv = unit_skill(u, sk);
        if (sv == NULL) {
            /* we can only learn skills we already have */
            cmistake(u, ord, 771, MSG_EVENT);
            return -1;
        }
    }

    /* snotlings koennen Talente nur bis T8 lernen */
    if (u_race(u) == rc_snotling) {
        if (get_level(u, sk) >= 8) {
            cmistake(u, ord, 308, MSG_EVENT);
            return -1;
        }
    }

    p = studycost = study_cost(u, sk);
    a = a_find(u->attribs, &at_learning);
    if (a != NULL) {
        teach = (teaching_info *)a->data.v;
    }

    /* keine kostenpflichtigen Talente fuer Migranten. Vertraute sind
     * keine Migranten, wird in is_migrant abgefangen. Vorsicht,
     * studycost darf hier noch nicht durch Akademie erhoeht sein */
    if (studycost > 0 && !ExpensiveMigrants() && is_migrant(u)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_migrants_nolearn",
            ""));
        return -1;
    }
    /* Akademie: */
    if (active_building(u, bt_find("academy"))) {
        studycost = MAX(50, studycost * 2);
    }

    if (sk == SK_MAGIC) {
        if (u->number > 1) {
            cmistake(u, ord, 106, MSG_MAGIC);
            return -1;
        }
        if (is_familiar(u)) {
            /* Vertraute zaehlen nicht zu den Magiern einer Partei,
             * koennen aber nur Graue Magie lernen */
            mtyp = M_GRAY;
        }
        else if (!has_skill(u, SK_MAGIC)) {
            int mmax = skill_limit(u->faction, SK_MAGIC);
            /* Die Einheit ist noch kein Magier */
            if (count_skill(u->faction, SK_MAGIC) + u->number > mmax) {
                ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_max_magicians",
                    "amount", mmax));
                return -1;
            }
            mtyp = getmagicskill(u->faction->locale);
            if (mtyp == M_NONE || mtyp == M_GRAY) {
                /* wurde kein Magiegebiet angegeben, wird davon
                 * ausgegangen, dass das normal gelernt werden soll */
                if (u->faction->magiegebiet != 0) {
                    mtyp = u->faction->magiegebiet;
                }
                else {
                    /* Es wurde kein Magiegebiet angegeben und die Partei
                     * hat noch keins gewaehlt. */
                    mtyp = getmagicskill(u->faction->locale);
                    if (mtyp == M_NONE) {
                        cmistake(u, ord, 178, MSG_MAGIC);
                        return -1;
                    }
                }
            }
            if (mtyp != u->faction->magiegebiet) {
                /* Es wurde versucht, ein anderes Magiegebiet zu lernen
                 * als das der Partei */
                if (u->faction->magiegebiet != 0) {
                    cmistake(u, ord, 179, MSG_MAGIC);
                    return -1;
                }
                else {
                    /* Lernt zum ersten mal Magie und legt damit das
                     * Magiegebiet der Partei fest */
                    u->faction->magiegebiet = mtyp;
                }
            }
            create_mage(u, mtyp);
        }
        else {
            /* ist schon ein Magier und kein Vertrauter */
            if (u->faction->magiegebiet == 0) {
                /* die Partei hat noch kein Magiegebiet gewaehlt. */
                mtyp = getmagicskill(u->faction->locale);
                if (mtyp == M_NONE) {
                    mtyp = getmagicskill(u->faction->locale);
                    if (mtyp == M_NONE) {
                        cmistake(u, ord, 178, MSG_MAGIC);
                        return -1;
                    }
                }
                /* Legt damit das Magiegebiet der Partei fest */
                u->faction->magiegebiet = mtyp;
            }
        }
    }
    if (sk == SK_ALCHEMY) {
        maxalchemy = effskill(u, SK_ALCHEMY, 0);
        if (!has_skill(u, SK_ALCHEMY)) {
            int amax = skill_limit(u->faction, SK_ALCHEMY);
            if (count_skill(u->faction, SK_ALCHEMY) + u->number > amax) {
                ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_max_alchemists",
                    "amount", amax));
                return -1;
            }
        }
    }
    if (studycost) {
        int cost = studycost * u->number;
        money = get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, cost);
        if (money > cost) money = cost;
    }
    if (money < studycost * u->number) {
        studycost = p;              /* Ohne Univertreurung */
        if (money > studycost) money = studycost;
        if (p > 0 && money < studycost * u->number) {
            cmistake(u, ord, 65, MSG_EVENT);
            multi = money / (double)(studycost * u->number);
        }
    }

    if (teach == NULL) {
        a = a_add(&u->attribs, a_new(&at_learning));
        teach = (teaching_info *)a->data.v;
        assert(teach);
        teach->teachers = NULL;
    }
    if (money > 0) {
        use_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, money);
        ADDMSG(&u->faction->msgs, msg_message("studycost",
            "unit region cost skill", u, u->region, money, sk));
    }

    if (get_effect(u, oldpotiontype[P_WISE])) {
        l = MIN(u->number, get_effect(u, oldpotiontype[P_WISE]));
        teach->days += l * EXPERIENCEDAYS;
        change_effect(u, oldpotiontype[P_WISE], -l);
    }
    if (get_effect(u, oldpotiontype[P_FOOL])) {
        l = MIN(u->number, get_effect(u, oldpotiontype[P_FOOL]));
        teach->days -= l * STUDYDAYS;
        change_effect(u, oldpotiontype[P_FOOL], -l);
    }

    if (p != studycost) {
        /* ist_in_gebaeude(r, u, BT_UNIVERSITAET) == 1) { */
        /* p ist Kosten ohne Uni, studycost mit; wenn
         * p!=studycost, ist die Einheit zwangsweise
         * in einer Uni */
        teach->days += u->number * EXPERIENCEDAYS;
    }

    if (is_cursed(r->attribs, &ct_badlearn)) {
        teach->days -= u->number * EXPERIENCEDAYS;
    }

    multi *= study_speedup(u, sk, speed_rule);
    days = study_days(u, sk);
    days = (int)((days + teach->days) * multi);

    /* the artacademy currently improves the learning of entertainment
       of all units in the region, to be able to make it cumulative with
       with an academy */

    if (sk == SK_ENTERTAINMENT
        && buildingtype_exists(r, bt_find("artacademy"), false)) {
        days *= 2;
    }

    if (fval(u, UFL_HUNGER))
        days /= 2;

    learn_skill(u, sk, days);
    if (a != NULL) {
        if (teach->teachers) {
            msg_teachers(teach->teachers, u, sk);
        }
        a_remove(&u->attribs, a);
        a = NULL;
    }
    u->flags |= (UFL_LONGACTION | UFL_NOTMOVING);

    /* Anzeigen neuer Traenke */
    /* Spruchlistenaktualiesierung ist in Regeneration */

    if (sk == SK_ALCHEMY) {
        const potion_type *ptype;
        faction *f = u->faction;
        int skill = effskill(u, SK_ALCHEMY, 0);
        if (skill > maxalchemy) {
            for (ptype = potiontypes; ptype; ptype = ptype->next) {
                if (skill == ptype->level * 2) {
                    a = a_find(f->attribs, &at_showitem);
                    while (a && a->type == &at_showitem && a->data.v != ptype)
                        a = a->next;
                    if (a == NULL || a->type != &at_showitem) {
                        a = a_add(&f->attribs, a_new(&at_showitem));
                        a->data.v = (void *)ptype->itype;
                    }
                }
            }
        }
    }
    else if (sk == SK_MAGIC) {
        sc_mage *mage = get_mage_depr(u);
        if (!mage) {
            mage = create_mage(u, u->faction->magiegebiet);
        }
    }
    init_order_depr(NULL);
    return 0;
}

static int produceexp_days(void) {
    static int config, rule;
    if (config_changed(&config)) {
        rule = config_get_int("study.produceexp", EXPERIENCEDAYS);
    }
    return rule;
}

void produceexp_ex(struct unit *u, skill_t sk, int n, learn_fun learn)
{
    assert(u && n <= u->number);
    if (n > 0 && (is_monsters(u->faction) || playerrace(u_race(u)))) {
        int days = produceexp_days();
        learn(u, sk, days * n);
    }
}

void produceexp(struct unit *u, skill_t sk, int n)
{
    produceexp_ex(u, sk, n, learn_skill);
}

static learn_fun inject_learn_fun = 0;

void inject_learn(learn_fun fun) {
    inject_learn_fun = fun;
}

/** days should be scaled by u->number; STUDYDAYS * u->number is one week worth of learning */
void learn_skill(unit *u, skill_t sk, int days) {
    int leveldays = STUDYDAYS * u->number;
    int weeks = 0;

    if (inject_learn_fun) {
        inject_learn_fun(u, sk, days);
        return;
    }
    while (days >= leveldays) {
        ++weeks;
        days -= leveldays;
    }
    if (days > 0 && rng_int() % leveldays < days) {
        ++weeks;
    }
    if (weeks > 0) {
        increase_skill(u, sk, weeks);
    }
}

void reduce_skill_days(unit *u, skill_t sk, int days) {
    skill *sv = unit_skill(u, sk);
    if (sv) {
        while (days > 0) {
            if (days >=  STUDYDAYS * u->number) {
                reduce_skill(u, sv, 1);
                days -= STUDYDAYS;
            }
            else {
                if (chance (days / ((double) STUDYDAYS * u->number))) /* (rng_int() % (30 * u->number) < days)*/
                    reduce_skill(u, sv, 1);
                days = 0;
            }
        }
    }
}

/** Talente von Dämonen verschieben sich.
*/
void demon_skillchange(unit *u)
{
    skill *sv = u->skills;
    int upchance = 15, downchance = 10;
    static int config;
    static bool rule_hunger;
    static int cfgup, cfgdown;

    if (config_changed(&config)) {
        rule_hunger = config_get_int("hunger.demon.skills", 0) != 0;
        cfgup = config_get_int("skillchange.demon.up", 15);
        cfgdown = config_get_int("skillchange.demon.down", 10);
    }
    if (cfgup == 0) {
        /* feature is disabled */
        return;
    }
    upchance = cfgup;
    downchance = cfgdown;

    if (fval(u, UFL_HUNGER)) {
        /* hungry demons only go down, never up in skill */
        if (rule_hunger) {
            downchance = upchance;
            upchance = 0;
        }
    }

    while (sv != u->skills + u->skill_size) {
        int roll = rng_int() % 100;
        if (sv->level > 0 && roll < upchance + downchance) {
            int weeks = 1 + rng_int() % 3;
            if (roll < downchance) {
                reduce_skill(u, sv, weeks);
                if (sv->level < 1) {
                    /* demons should never forget below 1 */
                    set_level(u, sv->id, 1);
                }
            }
            else {
                learn_skill(u, sv->id, STUDYDAYS * u->number * weeks);
            }
        }
        ++sv;
    }
}
