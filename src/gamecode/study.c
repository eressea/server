/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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

#define TEACH_ALL 1
#define TEACH_FRIENDS

#include <platform.h>
#include <kernel/config.h>
#include "study.h"

#include <kernel/alchemy.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/move.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/skill.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/language.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/umlaut.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TEACHNUMBER 10

static skill_t getskill(const struct locale *lang)
{
  return findskill(getstrtoken(), lang);
}

magic_t getmagicskill(const struct locale * lang)
{
  void **tokens = get_translations(lang, UT_MAGIC);
  variant token;
  const char *s = getstrtoken();

  if (tokens && s && s[0]) {
    if (findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
      return (magic_t) token.i;
    } else {
      char buffer[3];
      buffer[0] = s[0];
      buffer[1] = s[1];
      buffer[2] = '\0';
      if (findtoken(*tokens, buffer, &token) == E_TOK_SUCCESS) {
        return (magic_t) token.i;
      }
    }
  }
  return M_NONE;
}

/* ------------------------------------------------------------- */
/* Vertraute und Kröten sind keine Migranten */
boolean is_migrant(unit * u)
{
  if (u->race == u->faction->race)
    return false;

  if (fval(u->race, RCF_UNDEAD | RCF_ILLUSIONARY))
    return false;
  if (is_familiar(u))
    return false;
  if (u->race == new_race[RC_TOAD])
    return false;

  return true;
}

/* ------------------------------------------------------------- */
boolean magic_lowskill(unit * u)
{
  return (u->race == new_race[RC_TOAD]) ? true : false;
}

/* ------------------------------------------------------------- */

int study_cost(unit * u, skill_t sk)
{
  static int cost[MAXSKILLS];
  int stufe, k = 50;

  if (cost[sk] == 0) {
    char buffer[256];
    sprintf(buffer, "skills.cost.%s", skillnames[sk]);
    cost[sk] = get_param_int(global.parameters, buffer, -1);
  }
  if (cost[sk] >= 0) {
    return cost[sk];
  }
  switch (sk) {
  case SK_SPY:
    return 100;
  case SK_TACTICS:
  case SK_HERBALISM:
  case SK_ALCHEMY:
    return 200;
  case SK_MAGIC:               /* Die Magiekosten betragen 50+Summe(50*Stufe) */
    /* 'Stufe' ist dabei die nächste zu erreichende Stufe */
    stufe = 1 + get_level(u, SK_MAGIC);
    return k * (1 + ((stufe + 1) * stufe / 2));
  default:
    return 0;
  }
}

/* ------------------------------------------------------------- */

static void init_learning(struct attrib *a)
{
  a->data.v = calloc(sizeof(teaching_info), 1);
}

static void done_learning(struct attrib *a)
{
  free(a->data.v);
}

const attrib_type at_learning = {
  "learning",
  init_learning, done_learning, NULL, NULL, NULL,
  ATF_UNIQUE
};

static int study_days(unit * student, skill_t sk)
{
  int speed = 30;
  if (student->race->study_speed) {
    speed += student->race->study_speed[sk];
    if (speed < 30) {
      skill *sv = get_skill(student, sk);
      if (sv == 0) {
        speed = 30;
      }
    }
  }
  return student->number * speed;
}

static int
teach_unit(unit * teacher, unit * student, int nteaching, skill_t sk,
  boolean report, int *academy)
{
  teaching_info *teach = NULL;
  attrib *a;
  int n;

  /* learning sind die Tage, die sie schon durch andere Lehrer zugute
   * geschrieben bekommen haben. Total darf dies nicht über 30 Tage pro Mann
   * steigen.
   *
   * n ist die Anzahl zusätzlich gelernter Tage. n darf max. die Differenz
   * von schon gelernten Tagen zum MAX(30 Tage pro Mann) betragen. */

  if (magic_lowskill(student)) {
    cmistake(teacher, teacher->thisorder, 292, MSG_EVENT);
    return 0;
  }

  n = 30 * student->number;
  a = a_find(student->attribs, &at_learning);
  if (a != NULL) {
    teach = (teaching_info *) a->data.v;
    n -= teach->value;
  }

  n = MIN(n, nteaching);

  if (n != 0) {
    struct building *b = inside_building(teacher);
    const struct building_type *btype = b ? b->type : NULL;
    int index = 0;

    if (teach == NULL) {
      a = a_add(&student->attribs, a_new(&at_learning));
      teach = (teaching_info *) a->data.v;
    } else {
      while (teach->teachers[index] && index != MAXTEACHERS)
        ++index;
    }
    if (index < MAXTEACHERS)
      teach->teachers[index++] = teacher;
    if (index < MAXTEACHERS)
      teach->teachers[index] = NULL;
    teach->value += n;

    /* Solange Akademien größenbeschränkt sind, sollte Lehrer und
     * Student auch in unterschiedlichen Gebäuden stehen dürfen */
    if (btype == bt_find("academy")
      && student->building && student->building->type == bt_find("academy")) {
      int j = study_cost(student, sk);
      j = MAX(50, j * 2);
      /* kann Einheit das zahlen? */
      if (get_pooled(student, oldresourcetype[R_SILVER], GET_DEFAULT, j) >= j) {
        /* Jeder Schüler zusätzlich +10 Tage wenn in Uni. */
        teach->value += (n / 30) * 10;  /* learning erhöhen */
        /* Lehrer zusätzlich +1 Tag pro Schüler. */
        if (academy)
          *academy += n;
      }                         /* sonst nehmen sie nicht am Unterricht teil */
    }

    /* Teaching ist die Anzahl Leute, denen man noch was beibringen kann. Da
     * hier nicht n verwendet wird, werden die Leute gezählt und nicht die
     * effektiv gelernten Tage. -> FALSCH ? (ENNO)
     *
     * Eine Einheit A von 11 Mann mit Talent 0 profitiert vom ersten Lehrer B
     * also 10x30=300 tage, und der zweite Lehrer C lehrt für nur noch 1x30=30
     * Tage (damit das Maximum von 11x30=330 nicht überschritten wird).
     *
     * Damit es aber in der Ausführung nicht auf die Reihenfolge drauf ankommt,
     * darf der zweite Lehrer C keine weiteren Einheiten D mehr lehren. Also
     * wird student 30 Tage gutgeschrieben, aber teaching sinkt auf 0 (300-11x30 <=
     * 0).
     *
     * Sonst träte dies auf:
     *
     * A: lernt B: lehrt A C: lehrt A D D: lernt
     *
     * Wenn B vor C dran ist, lehrt C nur 30 Tage an A (wie oben) und
     * 270 Tage an D.
     *
     * Ist C aber vor B dran, lehrt C 300 tage an A, und 0 tage an D,
     * und B lehrt auch 0 tage an A.
     *
     * Deswegen darf C D nie lehren dürfen.
     *
     * -> Das ist wirr. wer hat das entworfen?
     * Besser wäre, man macht erst vorab alle zuordnungen, und dann
     * die Talentänderung (enno).
     */

    nteaching = MAX(0, nteaching - student->number * 30);

  }
  return n;
}

int teach_cmd(unit * u, struct order *ord)
{
  static const curse_type *gbdream_ct = NULL;
  plane *pl;
  region *r = u->region;
  int teaching, i, j, count, academy = 0;
  unit *u2;
  skill_t sk = NOSKILL;

  if (gbdream_ct == 0)
    gbdream_ct = ct_find("gbdream");
  if (gbdream_ct) {
    if (get_curse(u->region->attribs, gbdream_ct)) {
      ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "gbdream_noteach", ""));
      return 0;
    }
  }

  if ((u->race->flags & RCF_NOTEACH) || fval(u, UFL_WERE)) {
    cmistake(u, ord, 274, MSG_EVENT);
    return 0;
  }

  pl = rplane(r);
  if (pl && fval(pl, PFL_NOTEACH)) {
    cmistake(u, ord, 273, MSG_EVENT);
    return 0;
  }

  teaching = u->number * 30 * TEACHNUMBER;

  if ((i = get_effect(u, oldpotiontype[P_FOOL])) > 0) { /* Trank "Dumpfbackenbrot" */
    i = MIN(i, u->number * TEACHNUMBER);
    /* Trank wirkt pro Schüler, nicht pro Lehrer */
    teaching -= i * 30;
    change_effect(u, oldpotiontype[P_FOOL], -i);
    j = teaching / 30;
    ADDMSG(&u->faction->msgs, msg_message("teachdumb", "teacher amount", u, j));
  }
  if (teaching == 0)
    return 0;

  u2 = 0;
  count = 0;

  init_tokens(ord);
  skip_token();

#if TEACH_ALL
  if (getparam(u->faction->locale) == P_ANY) {
    unit *student = r->units;
    skill_t teachskill[MAXSKILLS];
    int i = 0;
    do {
      sk = getskill(u->faction->locale);
      teachskill[i++] = sk;
    } while (sk != NOSKILL);
    while (teaching && student) {
      if (student->faction == u->faction) {
#ifdef NEW_DAEMONHUNGER_RULE
        if (LongHunger(student))
          continue;
#else
        if (fval(student, UFL_HUNGER))
          continue;
#endif
        if (get_keyword(student->thisorder) == K_STUDY) {
          /* Input ist nun von student->thisorder !! */
          init_tokens(student->thisorder);
          skip_token();
          sk = getskill(student->faction->locale);
          if (sk != NOSKILL && teachskill[0] != NOSKILL) {
            for (i = 0; teachskill[i] != NOSKILL; ++i)
              if (sk == teachskill[i])
                break;
            sk = teachskill[i];
          }
          if (sk != NOSKILL
            && eff_skill_study(u, sk,
              r) - TEACHDIFFERENCE > eff_skill_study(student, sk, r)) {
            teaching -= teach_unit(u, student, teaching, sk, true, &academy);
          }
        }
      }
      student = student->next;
    }
#ifdef TEACH_FRIENDS
    while (teaching && student) {
      if (student->faction != u->faction
        && alliedunit(u, student->faction, HELP_GUARD)) {
#ifdef NEW_DAEMONHUNGER_RULE
        if (LongHunger(student))
          continue;
#else
        if (fval(student, UFL_HUNGER))
          continue;
#endif
        if (get_keyword(student->thisorder) == K_STUDY) {
          /* Input ist nun von student->thisorder !! */
          init_tokens(student->thisorder);
          skip_token();
          sk = getskill(student->faction->locale);
          if (sk != NOSKILL
            && eff_skill_study(u, sk, r) - TEACHDIFFERENCE >= eff_skill(student,
              sk, r)) {
            teaching -= teach_unit(u, student, teaching, sk, true, &academy);
          }
        }
      }
      student = student->next;
    }
#endif
  } else
#endif
  {
    char zOrder[4096];
    order *new_order;

    zOrder[0] = '\0';
    init_tokens(ord);
    skip_token();

    while (!parser_end()) {
      unit *u2 = getunit(r, u->faction);
      boolean feedback;
      ++count;

      /* Falls die Unit nicht gefunden wird, Fehler melden */

      if (!u2) {
        char tbuf[20];
        const char *uid;
        const char *token;
        /* Finde den string, der den Fehler verursacht hat */
        parser_pushstate();
        init_tokens(ord);
        skip_token();

        for (j = 0; j != count - 1; ++j) {
          /* skip over the first 'count' units */
          getunit(r, u->faction);
        }

        token = getstrtoken();

        /* Beginne die Fehlermeldung */
        if (isparam(token, u->faction->locale, P_TEMP)) {
          token = getstrtoken();
          sprintf(tbuf, "%s %s", LOC(u->faction->locale,
              parameters[P_TEMP]), token);
          uid = tbuf;
        } else {
          uid = token;
        }
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "unitnotfound_id",
            "id", uid));

        parser_popstate();
        continue;
      }

      feedback = u->faction == u2->faction
        || alliedunit(u2, u->faction, HELP_GUARD);

      /* Neuen Befehl zusammenbauen. TEMP-Einheiten werden automatisch in
       * ihre neuen Nummern übersetzt. */
      if (zOrder[0])
        strcat(zOrder, " ");
      strcat(zOrder, unitid(u2));

      if (get_keyword(u2->thisorder) != K_STUDY) {
        ADDMSG(&u->faction->msgs,
          msg_feedback(u, ord, "teach_nolearn", "student", u2));
        continue;
      }

      /* Input ist nun von u2->thisorder !! */
      parser_pushstate();
      init_tokens(u2->thisorder);
      skip_token();
      sk = getskill(u2->faction->locale);
      parser_popstate();

      if (sk == NOSKILL) {
        ADDMSG(&u->faction->msgs,
          msg_feedback(u, ord, "teach_nolearn", "student", u2));
        continue;
      }

      /* u is teacher, u2 is student */
      if (eff_skill_study(u2, sk, r) > eff_skill_study(u, sk,
          r) - TEACHDIFFERENCE) {
        if (feedback) {
          ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "teach_asgood",
              "student", u2));
        }
        continue;
      }
      if (sk == SK_MAGIC) {
        /* ist der Magier schon spezialisiert, so versteht er nur noch
         * Lehrer seines Gebietes */
        sc_mage *mage1 = get_mage(u);
        sc_mage *mage2 = get_mage(u2);
        if (!mage2 || !mage1 || (mage2->magietyp != M_GRAY
            && mage1->magietyp != mage2->magietyp)) {
          if (feedback) {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord,
                "error_different_magic", "target", u2));
          }
          continue;
        }
      }

      teaching -= teach_unit(u, u2, teaching, sk, false, &academy);
    }
    new_order = create_order(K_TEACH, u->faction->locale, "%s", zOrder);
    replace_order(&u->orders, ord, new_order);
    free_order(new_order);      /* parse_order & set_order have each increased the refcount */
  }
  if (academy && sk != NOSKILL) {
    academy = academy / 30;     /* anzahl gelehrter wochen, max. 10 */
    learn_skill(u, sk, academy / 30.0 / TEACHNUMBER);
  }
  return 0;
}

static double study_speedup(unit * u)
{
#define MINTURN 5               /* 5 */
#define OFSTURN 2               /* 2 */
  if (turn > MINTURN) {
    static int speed_rule = -1;
    if (speed_rule < 0) {
      speed_rule = get_param_int(global.parameters, "study.speedup", 0);
    }
    if (speed_rule == 1) {
      double learn_age = OFSTURN;
      int i;
      for (i = 0; i != u->skill_size; ++i) {
        skill *sv = u->skills + i;
        double learn_time = sv->level * (sv->level + 1) / 2.0;
        learn_age += learn_time;
      }
      if (learn_age < turn) {
        return 2.0 - learn_age / turn;
      }
    }
  }
  return 1.0;
}

int learn_cmd(unit * u, order * ord)
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
  static int learn_newskills = -1;
  if (learn_newskills < 0) {
    const char *str = get_param(global.parameters, "study.newskills");
    if (str && strcmp(str, "false") == 0)
      learn_newskills = 0;
    else
      learn_newskills = 1;
  }

  if ((u->race->flags & RCF_NOLEARN) || fval(u, UFL_WERE)) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_race_nolearn", "race",
        u->race));
    return 0;
  }

  init_tokens(ord);
  skip_token();
  sk = getskill(u->faction->locale);

  if (sk < 0) {
    cmistake(u, ord, 77, MSG_EVENT);
    return 0;
  }
  if (SkillCap(sk) && SkillCap(sk) <= effskill(u, sk)) {
    cmistake(u, ord, 771, MSG_EVENT);
    return 0;
  }
  /* Hack: Talente mit Malus -99 können nicht gelernt werden */
  if (u->race->bonus[sk] == -99) {
    cmistake(u, ord, 771, MSG_EVENT);
    return 0;
  }
  if (learn_newskills == 0) {
    skill *sv = get_skill(u, sk);
    if (sv == NULL) {
      /* we can only learn skills we already have */
      cmistake(u, ord, 771, MSG_EVENT);
      return 0;
    }
  }

  /* snotlings können Talente nur bis T8 lernen */
  if (u->race == new_race[RC_SNOTLING]) {
    if (get_level(u, sk) >= 8) {
      cmistake(u, ord, 308, MSG_EVENT);
      return 0;
    }
  }

  p = studycost = study_cost(u, sk);
  a = a_find(u->attribs, &at_learning);
  if (a != NULL) {
    teach = (teaching_info *) a->data.v;
  }

  /* keine kostenpflichtigen Talente für Migranten. Vertraute sind
   * keine Migranten, wird in is_migrant abgefangen. Vorsicht,
   * studycost darf hier noch nicht durch Akademie erhöht sein */
  if (studycost > 0 && !ExpensiveMigrants() && is_migrant(u)) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_migrants_nolearn",
        ""));
    return 0;
  }
  /* Akademie: */
  {
    struct building *b = inside_building(u);
    const struct building_type *btype = b ? b->type : NULL;

    if (btype && btype == bt_find("academy")) {
      studycost = MAX(50, studycost * 2);
    }
  }

  if (sk == SK_MAGIC) {
    if (u->number > 1) {
      cmistake(u, ord, 106, MSG_MAGIC);
      return 0;
    }
    if (is_familiar(u)) {
      /* Vertraute zählen nicht zu den Magiern einer Partei,
       * können aber nur Graue Magie lernen */
      mtyp = M_GRAY;
      if (!is_mage(u))
        create_mage(u, mtyp);
    } else if (!has_skill(u, SK_MAGIC)) {
      int mmax = skill_limit(u->faction, SK_MAGIC);
      /* Die Einheit ist noch kein Magier */
      if (count_skill(u->faction, SK_MAGIC) + u->number > mmax) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_max_magicians",
            "amount", mmax));
        return 0;
      }
      mtyp = getmagicskill(u->faction->locale);
      if (mtyp == M_NONE || mtyp == M_GRAY) {
        /* wurde kein Magiegebiet angegeben, wird davon
         * ausgegangen, daß das normal gelernt werden soll */
        if (u->faction->magiegebiet != 0) {
          mtyp = u->faction->magiegebiet;
        } else {
          /* Es wurde kein Magiegebiet angegeben und die Partei
           * hat noch keins gewählt. */
          mtyp = getmagicskill(u->faction->locale);
          if (mtyp == M_NONE) {
            cmistake(u, ord, 178, MSG_MAGIC);
            return 0;
          }
        }
      }
      if (mtyp != u->faction->magiegebiet) {
        /* Es wurde versucht, ein anderes Magiegebiet zu lernen
         * als das der Partei */
        if (u->faction->magiegebiet != 0) {
          cmistake(u, ord, 179, MSG_MAGIC);
          return 0;
        } else {
          /* Lernt zum ersten mal Magie und legt damit das
           * Magiegebiet der Partei fest */
          u->faction->magiegebiet = mtyp;
        }
      }
      if (!is_mage(u))
        create_mage(u, mtyp);
    } else {
      /* ist schon ein Magier und kein Vertrauter */
      if (u->faction->magiegebiet == 0) {
        /* die Partei hat noch kein Magiegebiet gewählt. */
        mtyp = getmagicskill(u->faction->locale);
        if (mtyp == M_NONE) {
          mtyp = getmagicskill(u->faction->locale);
          if (mtyp == M_NONE) {
            cmistake(u, ord, 178, MSG_MAGIC);
            return 0;
          }
        }
        /* Legt damit das Magiegebiet der Partei fest */
        u->faction->magiegebiet = mtyp;
      }
    }
  }
  if (sk == SK_ALCHEMY) {
    maxalchemy = eff_skill(u, SK_ALCHEMY, r);
    if (!has_skill(u, SK_ALCHEMY)) {
      int amax = skill_limit(u->faction, SK_ALCHEMY);
      if (count_skill(u->faction, SK_ALCHEMY) + u->number > amax) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_max_alchemists",
            "amount", amax));
        return 0;
      }
    }
  }
  if (studycost) {
    int cost = studycost * u->number;
    money = get_pooled(u, oldresourcetype[R_SILVER], GET_DEFAULT, cost);
    money = MIN(money, cost);
  }
  if (money < studycost * u->number) {
    studycost = p;              /* Ohne Univertreurung */
    money = MIN(money, studycost);
    if (p > 0 && money < studycost * u->number) {
      cmistake(u, ord, 65, MSG_EVENT);
      multi = money / (double)(studycost * u->number);
    }
  }

  if (teach == NULL) {
    a = a_add(&u->attribs, a_new(&at_learning));
    teach = (teaching_info *) a->data.v;
    teach->teachers[0] = 0;
  }
  if (money > 0) {
    use_pooled(u, oldresourcetype[R_SILVER], GET_DEFAULT, money);
    ADDMSG(&u->faction->msgs, msg_message("studycost",
        "unit region cost skill", u, u->region, money, sk));
  }

  if (get_effect(u, oldpotiontype[P_WISE])) {
    l = MIN(u->number, get_effect(u, oldpotiontype[P_WISE]));
    teach->value += l * 10;
    change_effect(u, oldpotiontype[P_WISE], -l);
  }
  if (get_effect(u, oldpotiontype[P_FOOL])) {
    l = MIN(u->number, get_effect(u, oldpotiontype[P_FOOL]));
    teach->value -= l * 30;
    change_effect(u, oldpotiontype[P_FOOL], -l);
  }

  if (p != studycost) {
    /* ist_in_gebaeude(r, u, BT_UNIVERSITAET) == 1) { */
    /* p ist Kosten ohne Uni, studycost mit; wenn
     * p!=studycost, ist die Einheit zwangsweise
     * in einer Uni */
    teach->value += u->number * 10;
  }

  if (is_cursed(r->attribs, C_BADLEARN, 0)) {
    teach->value -= u->number * 10;
  }

  multi *= study_speedup(u);
  days = study_days(u, sk);
  days = (int)((days + teach->value) * multi);

  /* the artacademy currently improves the learning of entertainment
     of all units in the region, to be able to make it cumulative with
     with an academy */

  if (sk == SK_ENTERTAINMENT
    && buildingtype_exists(r, bt_find("artacademy"), false)) {
    days *= 2;
  }

  if (fval(u, UFL_HUNGER))
    days /= 2;

  while (days) {
    if (days >= u->number * 30) {
      learn_skill(u, sk, 1.0);
      days -= u->number * 30;
    } else {
      double chance = (double)days / u->number / 30;
      learn_skill(u, sk, chance);
      days = 0;
    }
  }
  if (a != NULL) {
    if (teach != NULL) {
      int index = 0;
      while (teach->teachers[index] && index != MAXTEACHERS) {
        unit *teacher = teach->teachers[index++];
        if (teacher->faction != u->faction) {
          boolean feedback = alliedunit(u, teacher->faction, HELP_GUARD);
          if (feedback) {
            ADDMSG(&teacher->faction->msgs, msg_message("teach_teacher",
                "teacher student skill level", teacher, u, sk,
                effskill(u, sk)));
          }
          ADDMSG(&u->faction->msgs, msg_message("teach_student",
              "teacher student skill", teacher, u, sk));
        }
      }
    }
    a_remove(&u->attribs, a);
    a = NULL;
  }
  fset(u, UFL_LONGACTION | UFL_NOTMOVING);

  /* Anzeigen neuer Tränke */
  /* Spruchlistenaktualiesierung ist in Regeneration */

  if (sk == SK_ALCHEMY) {
    const potion_type *ptype;
    faction *f = u->faction;
    int skill = eff_skill(u, SK_ALCHEMY, r);
    if (skill > maxalchemy) {
      for (ptype = potiontypes; ptype; ptype = ptype->next) {
        if (skill == ptype->level * 2) {
          attrib *a = a_find(f->attribs, &at_showitem);
          while (a && a->type == &at_showitem && a->data.v != ptype)
            a = a->next;
          if (a == NULL || a->type != &at_showitem) {
            a = a_add(&f->attribs, a_new(&at_showitem));
            a->data.v = (void *)ptype->itype;
          }
        }
      }
    }
  } else if (sk == SK_MAGIC) {
    sc_mage *mage = get_mage(u);
    if (!mage) {
      mage = create_mage(u, u->faction->magiegebiet);
    }
  }

  return 0;
}
