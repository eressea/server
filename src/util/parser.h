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

  void init_tokens_str(const char *initstr, char *cmd);  /* initialize token parsing, take ownership of cmd */
  void skip_token(void);
  const char *parse_token(const char **str);
  void parser_pushstate(void);
  void parser_popstate(void);
  bool parser_end(void);
  const char *getstrtoken(void);
  int getid(void);
#define getshipid() getid()
#define getfactionid() getid()

#ifdef __cplusplus
}
#endif
#endif
