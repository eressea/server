/*
 +-------------------+
 |                   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 | Eressea PBEM host |  Enno Rehling <enno@eressea.de>
 | (c) 1998 - 2014   |  Katja Zedel <katze@felidae.kn-bremen.de>
 |                   |
 +-------------------+

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#include <platform.h>
#include <kernel/config.h>
#include "order.h"

#include "orderdb.h"
#include "skill.h"
#include "keyword.h"

#include <util/base36.h>
#include <util/language.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/strings.h>

#include <stream.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

# define ORD_KEYWORD(ord) (keyword_t)((ord)->command & 0xFFFF)
# define OD_STRING(odata) ((odata) ? (odata)->_str : NULL)

void replace_order(order ** dlist, order * orig, const order * src)
{
    assert(src);
    assert(orig);
    assert(dlist);
    while (*dlist != NULL) {
        order *dst = *dlist;
        if (dst->id == orig->id) {
            order *cpy = copy_order(src);
            *dlist = cpy;
            cpy->next = dst->next;
            dst->next = 0;
            free_order(dst);
        }
        dlist = &(*dlist)->next;
    }
}

keyword_t getkeyword(const order * ord)
{
    if (ord == NULL) {
        return NOKEYWORD;
    }
    return ORD_KEYWORD(ord);
}

/** returns a plain-text representation of the order.
 * This is the inverse function to the parse_order command. Note that
 * keywords are expanded to their full length.
 */
char* get_command(const order *ord, const struct locale *lang, char *sbuffer, size_t size) {
    order_data *od = NULL;
    const char * text;
    keyword_t kwd = ORD_KEYWORD(ord);
    sbstring sbs;

    sbs_init(&sbs, sbuffer, size);
    if (ord->command & CMD_QUIET) {
        sbs_strcpy(&sbs, "!");
    }
    if (ord->command & CMD_PERSIST) {
        sbs_strcat(&sbs, "@");
    }

    if (ord->id < 0) {
        skill_t sk = (skill_t)(100+ord->id);
        assert(kwd == K_STUDY && sk != SK_MAGIC && sk < MAXSKILLS);
        text = skillname(sk, lang);
    } else {
        od = odata_load(ord->id);
        text = OD_STRING(od);
    }
    if (kwd != NOKEYWORD) {
        const char *str = (const char *)LOC(lang, keyword(kwd));
        assert(str);
        sbs_strcat(&sbs, str);
        if (text) {
            sbs_strcat(&sbs, " ");
        }
    }
    if (text) {
        sbs_strcat(&sbs, text);
    }
    if (od) {
        odata_release(od);
    }
    return sbuffer;
}

int stream_order(struct stream *out, const struct order *ord, const struct locale *lang, bool escape)
{
    const char *str, *text;
    order_data *od = NULL;
    keyword_t kwd = ORD_KEYWORD(ord);

    if (ord->command & CMD_QUIET) {
        swrite("!", 1, 1, out);
    }
    if (ord->command & CMD_PERSIST) {
        swrite("@", 1, 1, out);
    }

    if (ord->id < 0) {
        skill_t sk = (skill_t)(100 + ord->id);
        assert(kwd == K_STUDY && sk != SK_MAGIC && sk < MAXSKILLS);
        text = skillname(sk, lang);
    }
    else {
        od = odata_load(ord->id);
        text = OD_STRING(od);
    }
    if (kwd != NOKEYWORD) {
        str = (const char *)LOC(lang, keyword(kwd));
        assert(str);
        swrite(str, 1, strlen(str), out);
    }

    if (text) {
        char obuf[1024];
        swrite(" ", 1, 1, out);
        if (escape) {
            text = str_escape(text, obuf, sizeof(obuf));
        }
        swrite(text, 1, strlen(text), out);
    }
    if (od) {
        odata_release(od);
    }

    return 0;
}

void free_order(order * ord)
{
    if (ord != NULL) {
        assert(ord->next == 0);
        free(ord);
    }
}

order *copy_order(const order * src)
{
    if (src != NULL) {
        order *ord = (order *)malloc(sizeof(order));
        ord->next = NULL;
        ord->command = src->command;
        ord->id = src->id;
        return ord;
    }
    return NULL;
}

void set_order(struct order **destp, struct order *src)
{
    if (*destp == src)
        return;
    free_order(*destp);
    *destp = src;
}

void free_orders(order ** olist)
{
    while (*olist) {
        order *ord = *olist;
        *olist = ord->next;
        ord->next = NULL;
        free_order(ord);
    }
}

static int create_data(keyword_t kwd, const char *s,
    const struct locale *lang)
{
    order_data *data;
    int id;

    assert(kwd!=NOKEYWORD);

    if (!s || *s == 0) {
        return 0;
    }
    if (kwd==K_STUDY) {
        const char * sptr = s;
        skill_t sk = get_skill(parse_token_depr(&sptr), lang);
        if (sk != SK_MAGIC && sk != NOSKILL) {
            return ((int)sk)-100;
        }
    }
    /* TODO: between mkdata and odata_release, this object is very
     * short-lived. */
    odata_create(&data, strlen(s), s);
    id = odata_save(data);
    odata_release(data);
    return id;
}

static void create_order_i(order *ord, keyword_t kwd, const char *sptr, bool persistent,
    bool noerror, const struct locale *lang)
{
    assert(ord);
    assert(kwd != NOKEYWORD && !keyword_disabled(kwd));

    ord->command = (int)kwd;
    if (persistent) ord->command |= CMD_PERSIST;
    if (noerror) ord->command |= CMD_QUIET;
    ord->next = NULL;

    while (isspace(*(unsigned char *)sptr)) ++sptr;

    ord->id = create_data(kwd, sptr, lang);
}

order *create_order(keyword_t kwd, const struct locale * lang,
    const char *params, ...)
{
    order *ord;
    char zBuffer[DISPLAYSIZE];
    if (params) {
        sbstring sbs;
        va_list marker;
        char *tok;

        assert(lang);
        va_start(marker, params);
        sbs_init(&sbs, zBuffer, sizeof(zBuffer));
        while (*params) {
            int i;
            const char *s;
            tok = strchr(params, '%');
            if (tok) {
                if (tok != params) {
                    sbs_strncat(&sbs, params, tok - params);
                }
                switch (tok[1]) {
                case 's':
                    s = va_arg(marker, const char *);
                    assert(s);
                    sbs_strcat(&sbs, s);
                    break;
                case 'd':
                    i = va_arg(marker, int);
                    sbs_strcat(&sbs, itoa10(i));
                    break;
                case 'i':
                    i = va_arg(marker, int);
                    sbs_strcat(&sbs, itoa36(i));
                    break;
                default:
                    assert(!"unknown format-character in create_order");
                }
                params = tok + 2;
            }
            else {
                sbs_strcat(&sbs, params);
                break;
            }
        }
        va_end(marker);
    }
    else {
        zBuffer[0] = 0;
    }
    ord = (order *)malloc(sizeof(order));
    create_order_i(ord, kwd, zBuffer, false, false, lang);
    return ord;
}

order *parse_order(const char *s, const struct locale * lang)
{
    assert(lang);
    assert(s);
    if (*s != 0) {
        keyword_t kwd = NOKEYWORD;
        const char *sptr = s;
        bool persistent = false, noerror = false;
        const char * p;

        p = *sptr ? parse_token_depr(&sptr) : 0;
        if (p) {
            while (*p == '!' || *p == '@') {
                if (*p == '!') noerror = true;
                else if (*p == '@') persistent = true;
                ++p;
            }
            kwd = get_keyword(p, lang);
        }
        if (kwd == K_MAKE) {
            const char *sp = sptr;
            p = parse_token_depr(&sp);
            if (p && isparam(p, lang, P_TEMP)) {
                kwd = K_MAKETEMP;
                sptr = sp;
            }
        }
        if (kwd != NOKEYWORD) {
            order *ord = (order *)malloc(sizeof(order));
            create_order_i(ord, kwd, sptr, persistent, noerror, lang);
            return ord;
        }
    }
    return NULL;
}

/**
 * Returns true if the order qualifies as "repeated". An order is repeated if it will overwrite the
 * old default order. K_BUY is in this category, but not K_MOVE.
 *
 * \param ord An order.
 * \return true if the order is long
 * \sa is_exclusive(), is_repeated(), is_persistent()
 */
bool is_repeated(keyword_t kwd)
{
    switch (kwd) {
    case K_CAST:
    case K_BUY:
    case K_SELL:
    case K_ROUTE:
    case K_DRIVE:
    case K_WORK:
    case K_BESIEGE:
    case K_ENTERTAIN:
    case K_TAX:
    case K_RESEARCH:
    case K_SPY:
    case K_STEAL:
    case K_SABOTAGE:
    case K_STUDY:
    case K_TEACH:
    case K_GROW:
    case K_PLANT:
    case K_PIRACY:
    case K_MAKE:
    case K_LOOT:
    case K_DESTROY:
        return true;

    default:
        break;
    }
    return false;
}

/**
 * Returns true if the order qualifies as "exclusive". An order is exclusive if it makes all other
 * long orders illegal. K_MOVE is in this category, but not K_BUY.
 *
 * \param ord An order.
 * \return true if the order is long
 * \sa is_exclusive(), is_repeated(), is_persistent()
 */
bool is_exclusive(const order * ord)
{
    keyword_t kwd = ORD_KEYWORD(ord);

    switch (kwd) {
    case K_MOVE:
    case K_ROUTE:
    case K_DRIVE:
    case K_WORK:
    case K_BESIEGE:
    case K_ENTERTAIN:
    case K_TAX:
    case K_RESEARCH:
    case K_SPY:
    case K_STEAL:
    case K_SABOTAGE:
    case K_STUDY:
    case K_TEACH:
    case K_GROW:
    case K_PLANT:
    case K_PIRACY:
    case K_MAKE:
    case K_LOOT:
    case K_DESTROY:
        return true;

    default:
        break;
    }
    return false;
}

/**
 * Returns true if the order qualifies as "long". An order is long if it excludes most other long
 * orders.
 *
 * \param ord An order.
 * \return true if the order is long
 * \sa is_exclusive(), is_repeated(), is_persistent()
 */
bool is_long(keyword_t kwd)
{
    switch (kwd) {
    case K_CAST:
    case K_BUY:
    case K_SELL:
    case K_MOVE:
    case K_ROUTE:
    case K_DRIVE:
    case K_WORK:
    case K_BESIEGE:
    case K_ENTERTAIN:
    case K_TAX:
    case K_RESEARCH:
    case K_SPY:
    case K_STEAL:
    case K_SABOTAGE:
    case K_STUDY:
    case K_TEACH:
    case K_GROW:
    case K_PLANT:
    case K_PIRACY:
    case K_MAKE:
    case K_LOOT:
    case K_DESTROY:
        return true;

    default:
        break;
    }
    return false;
}

/**
 * Returns true if the order qualifies as "persistent". An order is persistent if it will be
 * included in the template orders. @-orders, comments and most long orders are in this category,
 * but not K_MOVE.
 *
 * \param ord An order.
 * \return true if the order is persistent
 * \sa is_exclusive(), is_repeated(), is_persistent()
 */
bool is_persistent(const order * ord)
{
    keyword_t kwd = ORD_KEYWORD(ord);
    switch (kwd) {
    case K_MOVE:
    case NOKEYWORD:
        /* lang, aber niemals persistent! */
        return false;
    case K_KOMMENTAR:
        return true;
    default:
        return (ord->command & CMD_PERSIST) || is_repeated(kwd);
    }
}

bool is_silent(const order * ord)
{
    return (ord->command & CMD_QUIET) != 0;
}

char *write_order(const order * ord, const struct locale *lang, char *buffer, size_t size)
{
    if (ord == 0) {
        buffer[0] = 0;
    }
    else {
        keyword_t kwd = ORD_KEYWORD(ord);
        if (kwd == NOKEYWORD) {
            order_data *od = odata_load(ord->id);
            const char *text = OD_STRING(od);
            if (text) str_strlcpy(buffer, (const char *)text, size);
            else buffer[0] = 0;
            odata_release(od);
        }
        else {
            get_command(ord, lang, buffer, size);
        }
    }
    return buffer;
}

void push_order(order ** ordp, order * ord)
{
    while (*ordp)
        ordp = &(*ordp)->next;
    *ordp = ord;
}

static order_data *parser_od;

keyword_t init_order(const struct order *ord, const struct locale *lang)
{
    if (!ord) {
        odata_release(parser_od);
        parser_od = NULL;
        return NOKEYWORD;
    }
    else {
        keyword_t kwd = ORD_KEYWORD(ord);
        if (parser_od) {
            /* TODO: warning */
            odata_release(parser_od);
            parser_od = NULL;
        }
        if (ord->id < 0) {
            skill_t sk = (skill_t)(100 + ord->id);
            assert(sk < MAXSKILLS);
            assert(lang);
            assert(kwd == K_STUDY);
            init_tokens_str(skillname(sk, lang));
        }
        else {
            const char *str;
            parser_od = odata_load(ord->id);
            if (parser_od) {
                odata_addref(parser_od);
            }
            str = OD_STRING(parser_od);
            init_tokens_ex(str, parser_od, (void(*)(void *))odata_release);
        }
        return kwd;
    }
}

keyword_t init_order_depr(const struct order *ord)
{
    if (ord) {
        keyword_t kwd = ORD_KEYWORD(ord);
        assert(kwd != K_STUDY);
    }
    return init_order(ord, NULL);
}

void close_orders(void) {
    if (parser_od) {
        (void)init_order(NULL, NULL);
    }
}

