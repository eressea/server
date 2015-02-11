/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 
 */

#include <stdio.h>
#include <config.h>
#include <eressea.h>

static char *dwarf_syllable1[] = {
  "B", "D", "F", "G", "Gl", "H", "K", "L", "M", "N", "R", "S", "T", "Th", "V",
};

static char *dwarf_syllable2[] = {
  "a", "e", "i", "o", "oi", "u",
};

static char *dwarf_syllable3[] = {
  "bur", "fur", "gan", "gnus", "gnar", "li", "lin", "lir", "mli", "nar", "nus",
    "rin", "ran", "sin", "sil", "sur",
};

static char *elf_syllable1[] = {
  "Al", "An", "Bal", "Bel", "Cal", "Cel", "El", "Elr", "Elv", "Eow", "Ear", "F",
    "Fal", "Fel", "Fin", "G", "Gal", "Gel", "Gl", "Is", "Lan", "Leg", "Lom",
    "N", "Nal", "Nel", "S", "Sal", "Sel", "T", "Tal", "Tel", "Thr", "Tin",
};

static char *elf_syllable2[] = {
  "a", "adrie", "ara", "e", "ebri", "ele", "ere", "i", "io", "ithra", "ilma",
    "il-Ga", "ili", "o", "orfi", "u", "y",
};

static char *elf_syllable3[] = {
  "l", "las", "lad", "ldor", "ldur", "linde", "lith", "mir", "n", "nd", "ndel",
    "ndil", "ndir", "nduil", "ng", "mbor", "r", "rith", "ril", "riand", "rion",
    "s", "thien", "viel", "wen", "wyn",
};

static char *gnome_syllable1[] = {
  "Aar", "An", "Ar", "As", "C", "H", "Han", "Har", "Hel", "Iir", "J", "Jan",
    "Jar", "K", "L", "M", "Mar", "N", "Nik", "Os", "Ol", "P", "R", "S", "Sam",
    "San", "T", "Ter", "Tom", "Ul", "V", "W", "Y",
};

static char *gnome_syllable2[] = {
  "a", "aa", "ai", "e", "ei", "i", "o", "uo", "u", "uu",
};

static char *gnome_syllable3[] = {
  "ron", "re", "la", "ki", "kseli", "ksi", "ku", "ja", "ta", "na", "namari",
    "neli", "nika", "nikki", "nu", "nukka", "ka", "ko", "li", "kki", "rik",
    "po", "to", "pekka", "rjaana", "rjatta", "rjukka", "la", "lla", "lli", "mo",
    "nni",
};

static char *hobbit_syllable1[] = {
  "B", "Ber", "Br", "D", "Der", "Dr", "F", "Fr", "G", "H", "L", "Ler", "M",
    "Mer", "N", "P", "Pr", "Per", "R", "S", "T", "W",
};

static char *hobbit_syllable2[] = {
  "a", "e", "i", "ia", "o", "oi", "u",
};

static char *hobbit_syllable3[] = {
  "bo", "ck", "decan", "degar", "do", "doc", "go", "grin", "lba", "lbo", "lda",
    "ldo", "lla", "ll", "lo", "m", "mwise", "nac", "noc", "nwise", "p", "ppin",
    "pper", "tho", "to",
};

static char *human_syllable1[] = {
  "Ab", "Ac", "Ad", "Af", "Agr", "Ast", "As", "Al", "Adw", "Adr", "Ar", "B",
    "Br", "C", "Cr", "Ch", "Cad", "D", "Dr", "Dw", "Ed", "Eth", "Et", "Er",
    "El", "Eow", "F", "Fr", "G", "Gr", "Gw", "Gal", "Gl", "H", "Ha", "Ib",
    "Jer", "K", "Ka", "Ked", "L", "Loth", "Lar", "Leg", "M", "Mir", "N", "Nyd",
    "Ol", "Oc", "On", "P", "Pr", "R", "Rh", "S", "Sev", "T", "Tr", "Th", "V",
    "Y", "Z", "W", "Wic",
};

static char *human_syllable2[] = {
  "a", "ae", "au", "ao", "are", "ale", "ali", "ay", "ardo", "e", "ei", "ea",
    "eri", "era", "ela", "eli", "enda", "erra", "i", "ia", "ie", "ire", "ira",
    "ila", "ili", "ira", "igo", "o", "oa", "oi", "oe", "ore", "u", "y",
};

static char *human_syllable3[] = {
  "a", "and", "b", "bwyn", "baen", "bard", "c", "ctred", "cred", "ch", "can",
    "d", "dan", "don", "der", "dric", "dfrid", "dus", "f", "g", "gord", "gan",
    "l", "li", "lgrin", "lin", "lith", "lath", "loth", "ld", "ldric", "ldan",
    "m", "mas", "mos", "mar", "mond", "n", "nydd", "nidd", "nnon", "nwan",
    "nyth", "nad", "nn", "nnor", "nd", "p", "r", "ron", "rd", "s", "sh", "seth",
    "sean", "t", "th", "tha", "tlan", "trem", "tram", "v", "vudd", "w", "wan",
    "win", "wyn", "wyr", "wyr", "wyth",
};

static char *orc_syllable1[] = {
  "B", "Er", "G", "Gr", "H", "P", "Pr", "R", "V", "Vr", "T", "Tr", "M", "Dr",
};

static char *orc_syllable2[] = {
  "a", "i", "o", "oo", "u", "ui",
};

static char *orc_syllable3[] = {
  "dash", "dish", "dush", "gar", "gor", "gdush", "lo", "gdish", "k", "lg",
    "nak", "rag", "rbag", "rg", "rk", "ng", "nk", "rt", "ol", "urk", "shnak",
    "mog", "mak", "rak",
};

static char *entish_syllable1[] = {
  "Baum", "Wurzel", "Rinden", "Ast", "Blatt",
};

static char *entish_syllable2[] = {
  "-",
};

static char *entish_syllable3[] = {
  "Hüter", "Pflanzer", "Hirte", "Wächter", "Wachser", "Beschützer",
};

static char *cthuloid_syllable1[] = {
  "Cth", "Az", "Fth", "Ts", "Xo", "Q'N", "R'L", "Ghata", "L", "Zz", "Fl", "Cl",
    "S", "Y",
};

static char *cthuloid_syllable2[] = {
  "nar", "loi", "ul", "lu", "noth", "thon", "ath", "'N", "rhy", "oth", "aza",
    "agn", "oa", "og",
};

static char *cthuloid_syllable3[] = {
  "l", "a", "u", "oa", "oggua", "oth", "ath", "aggua", "lu", "lo", "loth",
    "lotha", "agn", "axl",
};

static char *create_random_name(race_t race)
{
  static char name[64];

  switch (race) {
    case RC_DWARF:
      strcpy(name,
        dwarf_syllable1[rng_int() % (sizeof(dwarf_syllable1) /
            sizeof(char *))]);
      strcat(name,
        dwarf_syllable2[rand() % (sizeof(dwarf_syllable2) / sizeof(char *))]);
      strcat(name,
        dwarf_syllable3[rand() % (sizeof(dwarf_syllable3) / sizeof(char *))]);
      break;
    case RC_ELF:
      strcpy(name,
        elf_syllable1[rand() % (sizeof(elf_syllable1) / sizeof(char *))]);
      strcat(name,
        elf_syllable2[rand() % (sizeof(elf_syllable2) / sizeof(char *))]);
      strcat(name,
        elf_syllable3[rand() % (sizeof(elf_syllable3) / sizeof(char *))]);
      break;
/*
	case RACE_GNOME:
		strcpy(name, gnome_syllable1[rand()%(sizeof(gnome_syllable1) / sizeof(char*))]);
		strcat(name, gnome_syllable2[rand()%(sizeof(gnome_syllable2) / sizeof(char*))]);
		strcat(name, gnome_syllable3[rand()%(sizeof(gnome_syllable3) / sizeof(char*))]);
		break;
*/
    case RC_HALFLING:
      strcpy(name,
        hobbit_syllable1[rand() % (sizeof(hobbit_syllable1) / sizeof(char *))]);
      strcat(name,
        hobbit_syllable2[rand() % (sizeof(hobbit_syllable2) / sizeof(char *))]);
      strcat(name,
        hobbit_syllable3[rand() % (sizeof(hobbit_syllable3) / sizeof(char *))]);
      break;
    case RC_HUMAN:
    case RC_AQUARIAN:
    case RC_CAT:
      strcpy(name,
        human_syllable1[rand() % (sizeof(human_syllable1) / sizeof(char *))]);
      strcat(name,
        human_syllable2[rand() % (sizeof(human_syllable2) / sizeof(char *))]);
      strcat(name,
        human_syllable3[rand() % (sizeof(human_syllable3) / sizeof(char *))]);
      break;
    case RC_ORC:
    case RC_TROLL:
    case RC_GOBLIN:
      strcpy(name,
        orc_syllable1[rand() % (sizeof(orc_syllable1) / sizeof(char *))]);
      strcat(name,
        orc_syllable2[rand() % (sizeof(orc_syllable2) / sizeof(char *))]);
      strcat(name,
        orc_syllable3[rand() % (sizeof(orc_syllable3) / sizeof(char *))]);
      break;
/*
	case RC_TREEMAN:
		strcpy(name, entish_syllable1[rand()%(sizeof(entish_syllable1) / sizeof(char*))]);
		strcat(name, entish_syllable2[rand()%(sizeof(entish_syllable2) / sizeof(char*))]);
		strcat(name, entish_syllable3[rand()%(sizeof(entish_syllable3) / sizeof(char*))]);
		break;
*/
    case RC_DAEMON:
    case RC_INSECT:
      strcpy(name,
        cthuloid_syllable1[rand() % (sizeof(cthuloid_syllable1) /
            sizeof(char *))]);
      strcat(name,
        cthuloid_syllable2[rand() % (sizeof(cthuloid_syllable2) /
            sizeof(char *))]);
      strcat(name,
        cthuloid_syllable3[rand() % (sizeof(cthuloid_syllable3) /
            sizeof(char *))]);
      break;
    default:
      name[0] = 0;
      break;
  }

  return name;
}

int main(void)
{
  race_t race;

  for (race = 0; race < 11; race++) {
    int i;
    printf("%d:", (int)race);
    for (i = 0; i < 20; i++) {
      printf(" %s", create_random_name(race));
    }
    printf("\n");
  }

  return 0;
}
