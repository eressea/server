#ifndef H_KEYWORD_H
#define H_KEYWORD_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct locale;

    typedef enum keyword_t {
        K_KOMMENTAR,
        K_BANNER,
        K_WORK,
        K_ATTACK,
        K_STEAL,
        K_BESIEGE_UNUSED,
        K_NAME,
        K_USE,
        K_DISPLAY,
        K_ENTER,
        K_GUARD,
        K_MAIL,
        K_END,
        K_DRIVE,
        K_NUMBER,
        K_FOLLOW,
        K_RESEARCH,
        K_GIVE,
        K_ALLY,
        K_STATUS,
        K_COMBATSPELL,
        K_BUY,
        K_CONTACT,
        K_TEACH,
        K_STUDY,
        K_MAKE,
        K_MAKETEMP,
        K_MOVE,
        K_PASSWORD,
        K_RECRUIT,
        K_RESERVE,
        K_ROUTE,
        K_SABOTAGE,
        K_SEND,
        K_SPY,
        K_QUIT,
        K_SETSTEALTH,
        K_TRANSPORT,
        K_TAX,
        K_ENTERTAIN,
        K_SELL,
        K_LEAVE,
        K_FORGET,
        K_CAST,
        K_RESHOW,
        K_DESTROY,
        K_PLANT,
        K_GROW,
        K_DEFAULT,
        K_URSPRUNG,
        K_EMAIL,
        K_PIRACY,
        K_GROUP,
        K_SORT,
        K_PREFIX,
        K_ALLIANCE,
        K_CLAIM,
        K_PROMOTION,
        K_PAY,
        K_LOOT,
        K_AUTOSTUDY,
        MAXKEYWORDS,
        NOKEYWORD
    } keyword_t;

    extern const char *keywords[MAXKEYWORDS];

    keyword_t findkeyword(const char *s);
    keyword_t get_keyword(const char *s, const struct locale *lang);
    void init_keywords(const struct locale *lang);
    void init_keyword(const struct locale *lang, keyword_t kwd, const char *str);
    bool keyword_disabled(keyword_t kwd);
    const char *keyword_name(keyword_t kwd, const struct locale *lang);
    void enable_keyword(keyword_t kwd, bool enabled);
    const char *keyword(keyword_t kwd);

#ifdef __cplusplus
}
#endif
#endif
