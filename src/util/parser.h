/* 
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2007   |
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *
 */

#ifndef UTIL_PARSER_H
#define UTIL_PARSER_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    void init_tokens_ex(const char *initstr, void *data, void(*dtor)(void *));
    void init_tokens_str(const char *initstr);  /* initialize token parsing */
    void skip_token(void);
    const char *parse_token_depr(const char **str);
    char *parse_token(const char **str, char *buffer, size_t bufsize);
    void parser_pushstate(void);
    void parser_popstate(void);
    bool parser_end(void);
    const char *getstrtoken(void);
    const char *gettoken(char *lbuf, size_t bufsize);
    int getuint(void);
    int getint(void);
    int getid(void);
    unsigned int atoip(const char *s);

#ifdef __cplusplus
}
#endif
#endif
