/* vi: set ts=2:
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
#ifdef __cplusplus
extern "C" {
#endif

  extern void init_tokens_str(const char *initstr, char *cmd);  /* initialize token parsing, take ownership of cmd */
  extern void skip_token(void);
  extern const char *parse_token(const char **str);
  extern void parser_pushstate(void);
  extern void parser_popstate(void);
  extern bool parser_end(void);
  extern const char *getstrtoken(void);

#ifdef __cplusplus
}
#endif
#endif
