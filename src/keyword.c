#include <platform.h>
#include <kernel/config.h>
#include "keyword.h"

#include "util/language.h"
#include "util/umlaut.h"

#include <critbit.h>

#include <assert.h>
#include <string.h>

static const char * keyword_key(int i)
{
  assert(i<MAXKEYWORDS&& i>=0);
  return keywords[i];
}

void init_keyword(const struct locale *lang, keyword_t kwd, const char *str) {
    void **tokens = get_translations(lang, UT_KEYWORDS);
    struct critbit_tree **cb = (critbit_tree **)tokens;
    add_translation(cb, str, (int)kwd);
}

keyword_t findkeyword(const char *s) {
    int i;
    for (i=0;i!=MAXKEYWORDS;++i) {
        if (strcmp(s, keywords[i])==0) {
            return (keyword_t)i;
        }
    }
    return NOKEYWORD;
}


void init_keywords(const struct locale *lang) {
    init_translations(lang, UT_KEYWORDS, keyword_key, MAXKEYWORDS);
}

keyword_t get_keyword(const char *s, const struct locale *lang) {
    keyword_t result = NOKEYWORD;
    char buffer[64];

    assert(lang);
    assert(s);
    while (*s == '@') ++s;

    if (*s) {
        char *str = transliterate(buffer, sizeof(buffer) - sizeof(int), s);

        if (str) {
            int i;
            const void *match;
            void **tokens = get_translations(lang, UT_KEYWORDS);
            critbit_tree *cb = (critbit_tree *)*tokens;
            assert(cb);
            if (cb_find_prefix(cb, str, strlen(str), &match, 1, 0)) {
                cb_get_kv(match, &i, sizeof(int));
                result = (keyword_t)i;
                return keyword_disabled(result) ? NOKEYWORD : result;
            }
        }
    }
    return NOKEYWORD;
}

static bool disabled_kwd[MAXKEYWORDS];

void enable_keyword(keyword_t kwd, bool enabled) {
    assert(kwd<MAXKEYWORDS);
    disabled_kwd[kwd] = !enabled;
}

bool keyword_disabled(keyword_t kwd) {
    assert(kwd<MAXKEYWORDS);
    return disabled_kwd[kwd];
}

const char *keywords[MAXKEYWORDS] = {
  "//",
  "BANNER",
  "ARBEITEN",
  "ATTACKIEREN",
  "BEKLAUEN",
  "BELAGERE",
  "BENENNEN",
  "BENUTZEN",
  "BESCHREIBEN",
  "BETRETEN",
  "BEWACHEN",
  "BOTSCHAFT",
  "ENDE",
  "FAHREN",
  "NUMMER",
  "FOLGEN",
  "FORSCHEN",
  "GIB",
  "HELFEN",
  "KAEMPFEN",
  "KAMPFZAUBER",
  "KAUFEN",
  "KONTAKTIEREN",
  "LEHREN",
  "LERNEN",
  "MACHEN",
  "NACH",
  "PASSWORT",
  "REKRUTIEREN",
  "RESERVIEREN",
  "ROUTE",
  "SABOTIEREN",
  "OPTION",
  "SPIONIEREN",
  "STIRB",
  "TARNEN",
  "TRANSPORTIEREN",
  "TREIBEN",
  "UNTERHALTEN",
  "VERKAUFEN",
  "VERLASSEN",
  "VERGESSEN",
  "ZAUBERE",
  "ZEIGEN",
  "ZERSTOEREN",
  "ZUECHTEN",
  "DEFAULT",
  "URSPRUNG",
  "EMAIL",
  "PIRATERIE",
  "GRUPPE",
  "SORTIEREN",
  "PRAEFIX",
  "PFLANZEN",
  "ALLIANZ",
  "BEANSPRUCHEN",
  "PROMOTION",
  "BEZAHLEN",
};

