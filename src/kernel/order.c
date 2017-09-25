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

#include "skill.h"
#include "keyword.h"

#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/language.h>
#include <util/log.h>
#include <util/parser.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

# define ORD_KEYWORD(ord) (ord)->data->_keyword
# define ORD_LOCALE(ord) locale_array[(ord)->data->_lindex]->lang
# define ORD_STRING(ord) (ord)->data->_str

typedef struct locale_data {
    struct order_data *short_orders[MAXKEYWORDS];
    struct order_data *study_orders[MAXSKILLS];
    const struct locale *lang;
} locale_data;

static struct locale_data *locale_array[MAXLOCALES];

typedef struct order_data {
    const char *_str;
    int _refcount;
    int _lindex;
    keyword_t _keyword;
} order_data;

static void release_data(order_data * data)
{
    if (data) {
        if (--data->_refcount == 0) {
            free(data);
        }
    }
}

void replace_order(order ** dlist, order * orig, const order * src)
{
    assert(src);
    assert(orig);
    assert(dlist);
    while (*dlist != NULL) {
        order *dst = *dlist;
        if (dst->data == orig->data) {
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
char* get_command(const order *ord, char *sbuffer, size_t size) {
    char *bufp = sbuffer;
    const char *text = ORD_STRING(ord);
    keyword_t kwd = ORD_KEYWORD(ord);
    int bytes;

    if (ord->_noerror) {
        if (size > 0) {
            *bufp++ = '!';
            --size;
        }
        else {
            WARN_STATIC_BUFFER();
        }
    }
    if (ord->_persistent) {
        if (size > 0) {
            *bufp++ = '@';
            --size;
        }
        else {
            WARN_STATIC_BUFFER();
        }
    }
    if (kwd != NOKEYWORD) {
        const struct locale *lang = ORD_LOCALE(ord);
        if (size > 0) {
            const char *str = (const char *)LOC(lang, keyword(kwd));
            assert(str);
            if (text) --size;
            bytes = (int)strlcpy(bufp, str, size);
            if (wrptr(&bufp, &size, bytes) != 0) {
                WARN_STATIC_BUFFER();
            }
            if (text) *bufp++ = ' ';
        }
        else {
            WARN_STATIC_BUFFER();
        }
    }
    if (text) {
        bytes = (int)strlcpy(bufp, (const char *)text, size);
        if (wrptr(&bufp, &size, bytes) != 0) {
            WARN_STATIC_BUFFER();
            if (bufp - sbuffer >= 6) {
                bufp -= 6;
                while (bufp > sbuffer && (*bufp & 0x80) != 0) {
                    ++size;
                    --bufp;
                }
                memcpy(bufp, "[...]", 6);   /* TODO: make sure this only happens in eval_command */
                bufp += 6;
            }
        }
    }
    if (size > 0) *bufp = 0;
    return sbuffer;
}

void free_order(order * ord)
{
    if (ord != NULL) {
        assert(ord->next == 0);

        release_data(ord->data);
        free(ord);
    }
}

order *copy_order(const order * src)
{
    if (src != NULL) {
        order *ord = (order *)malloc(sizeof(order));
        ord->next = NULL;
        ord->_persistent = src->_persistent;
        ord->_noerror = src->_noerror;
        ord->data = src->data;
        ++ord->data->_refcount;
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

static char *mkdata(order_data **pdata, size_t len, keyword_t kwd, int lindex, const char *str)
{
    order_data *data;
    char *result;
    data = malloc(sizeof(order_data) + len + 1);
    result = (char *)(data + 1);
    data->_keyword = kwd;
    data->_lindex = lindex;
    data->_refcount = 0;
    data->_str = 0;
    data->_str = (len > 0) ? result : 0;
    if (str) strcpy(result, str);
    if (pdata) *pdata = data;
    return result;
}

static order_data *create_data(keyword_t kwd, const char *sptr, int lindex)
{
    const char *s = sptr;
    order_data *data;
    const struct locale *lang = locale_array[lindex]->lang;

    if (kwd != NOKEYWORD)
        s = (*sptr) ? sptr : NULL;

    /* learning, only one order_data per skill required */
    if (kwd == K_STUDY) {
        skill_t sk = get_skill(parse_token_depr(&sptr), lang);
        switch (sk) {
        case NOSKILL:              /* fehler */
            break;
        case SK_MAGIC:             /* kann parameter haben */
            if (*sptr != 0)
                break;
        default:                   /* nur skill als Parameter, keine extras */
            data = locale_array[lindex]->study_orders[sk];
            if (data == NULL) {
                const char *skname = skillname(sk, lang);
                const char *spc = strchr(skname, ' ');
                size_t len = strlen(skname);
                char *dst = mkdata(&data, len + (spc ? 3 : 0), kwd, lindex, spc ? 0 : skname);
                locale_array[lindex]->study_orders[sk] = data;
                if (spc) {
                    dst[0] = '\"';
                    memcpy(dst + 1, skname, len);
                    dst[len + 1] = '\"';
                    dst[len + 2] = '\0';
                }
                data->_refcount = 1;
            }
            ++data->_refcount;
            return data;
        }
    }

    /* orders with no parameter, only one order_data per order required */
    else if (kwd != NOKEYWORD && *sptr == 0) {
        data = locale_array[lindex]->short_orders[kwd];
        if (data == NULL) {
            mkdata(&data, 0, kwd, lindex, 0);
            data->_refcount = 1;
            locale_array[lindex]->short_orders[kwd] = data;
        }
        ++data->_refcount;
        return data;
    }
    mkdata(&data, s ? strlen(s) : 0, kwd, lindex, s);
    data->_refcount = 1;
    return data;
}

static void clear_localedata(int lindex) {
    int i;
    for (i = 0; i != MAXKEYWORDS; ++i) {
        release_data(locale_array[lindex]->short_orders[i]);
        locale_array[lindex]->short_orders[i] = 0;
    }
    for (i = 0; i != MAXSKILLS; ++i) {
        release_data(locale_array[lindex]->study_orders[i]);
        locale_array[lindex]->study_orders[i] = 0;
    }
    locale_array[lindex]->lang = 0;
}

void close_orders(void) {
    int i;
    for (i = 0; i != MAXLOCALES; ++i) {
        if (locale_array[i]){
            clear_localedata(i);
            free(locale_array[i]);
            locale_array[i] = 0;
        }
    }
}

static order *create_order_i(order *ord, keyword_t kwd, const char *sptr, bool persistent,
    bool noerror, const struct locale *lang)
{
    int lindex;

    assert(ord);
    if (kwd == NOKEYWORD || keyword_disabled(kwd)) {
        log_error("trying to create an order for disabled keyword %s.", keyword(kwd));
        return NULL;
    }

    /* if this is just nonsense, then we skip it. */
    if (lomem) {
        switch (kwd) {
        case K_KOMMENTAR:
        case NOKEYWORD:
            return NULL;
        default:
            break;
        }
    }

    lindex = locale_index(lang);
    assert(lindex < MAXLOCALES);
    if (!locale_array[lindex]) {
        locale_array[lindex] = (locale_data *)calloc(1, sizeof(locale_data));
    }
    else if (locale_array[lindex]->lang != lang) {
        clear_localedata(lindex);
    }
    locale_array[lindex]->lang = lang;

    ord->_persistent = persistent;
    ord->_noerror = noerror;
    ord->next = NULL;

    while (isspace(*(unsigned char *)sptr)) ++sptr;
    ord->data = create_data(kwd, sptr, lindex);

    return ord;
}

order *create_order(keyword_t kwd, const struct locale * lang,
    const char *params, ...)
{
    order *ord;
    char zBuffer[DISPLAYSIZE];
    if (params) {
        char *bufp = zBuffer;
        int bytes;
        size_t size = sizeof(zBuffer) - 1;
        va_list marker;

        assert(lang);
        va_start(marker, params);
        while (*params) {
            if (*params == '%') {
                int i;
                const char *s;
                ++params;
                switch (*params) {
                case 's':
                    s = va_arg(marker, const char *);
                    assert(s);
                    bytes = (int)strlcpy(bufp, s, size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    break;
                case 'd':
                    i = va_arg(marker, int);
                    bytes = (int)strlcpy(bufp, itoa10(i), size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    break;
                case 'i':
                    i = va_arg(marker, int);
                    bytes = (int)strlcpy(bufp, itoa36(i), size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                    break;
                default:
                    assert(!"unknown format-character in create_order");
                }
            }
            else if (size > 0) {
                *bufp++ = *params;
                --size;
            }
            ++params;
        }
        va_end(marker);
        *bufp = 0;
    }
    else {
        zBuffer[0] = 0;
    }
    ord = (order *)malloc(sizeof(order));
    return create_order_i(ord, kwd, zBuffer, false, false, lang);
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
            return create_order_i(ord, kwd, sptr, persistent, noerror, lang);
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
        return ord->_persistent || is_repeated(kwd);
    }
}

char *write_order(const order * ord, char *buffer, size_t size)
{
    if (ord == 0) {
        buffer[0] = 0;
    }
    else {
        keyword_t kwd = ORD_KEYWORD(ord);
        if (kwd == NOKEYWORD) {
            const char *text = ORD_STRING(ord);
            if (text) strlcpy(buffer, (const char *)text, size);
            else buffer[0] = 0;
        }
        else {
            get_command(ord, buffer, size);
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

keyword_t init_order(const struct order *ord)
{
    assert(ord && ord->data);
    init_tokens_str(ord->data->_str);
    return ord->data->_keyword;
}
