/* vi: set ts=2:
 +-------------------+  
 |                   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 | Eressea PBEM host |  Enno Rehling <enno@eressea-pbem.de>
 | (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
 |                   |
 +-------------------+  

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include "eressea.h"

#include "order.h"

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const struct locale * locale_array[16];
static int nlocales = 0;

#undef SHORT_STRINGS

void
copy_order(order * dst, const order * src)
{
  if (dst->_str) free(dst->_str);
  dst->_str = strdup(src->_str);
  dst->_keyword = src->_keyword;
  dst->_lindex = src->_lindex;
  dst->_persistent = src->_persistent;
}

keyword_t
get_keyword(const order * ord)
{
  if (ord==NULL) {
    return NOKEYWORD;
  }
  return ord->_keyword;
}

char *
getcommand(const order * ord)
{
  char sbuffer[DISPLAYSIZE*2];
  char * str = sbuffer;
  
  assert(ord->_lindex<nlocales);
  if (ord->_persistent) *str++ = '@';
#ifdef SHORT_STRINGS
  if (ord->_keyword!=NOKEYWORD) {
    const struct locale * lang = locale_array[ord->_lindex];

    strcpy(str, LOC(lang, keywords[ord->_keyword]));
    str += strlen(str);
    if (ord->_str) {
      *str++ = ' ';
      *str = 0;
    }
  }
#endif
  strcpy(str, ord->_str);
  return strdup(sbuffer);
}

void 
free_order(order * ord)
{
	if (ord!=NULL && --ord->_refcount==0) {
    assert(ord->next==NULL);
		if (ord->_str!=NULL) free(ord->_str);
		free(ord);
	}
}

order *
duplicate_order(order * ord)
{
	if (ord!=NULL) ++ord->_refcount;
	return ord;
}

void 
set_order(struct order ** destp, struct order * src)
{
  if (*destp==src) return;
  free_order(*destp);
  *destp = duplicate_order(src);
}

void
free_orders(order ** olist)
{
	while (*olist) {
		order * ord = *olist;
		*olist = ord->next;
    ord->next = NULL;
		free_order(ord);
	}
}

order * 
parse_order(const char * s, const struct locale * lang)
{
  while (*s && !isalnum(*(unsigned char*)s) && !ispunct(*(unsigned char*)s)) ++s;
  if (*s==0) return NULL;
  else {
    keyword_t kwd;
    const char * sptr;
    order * ord = NULL;
    int persistent = 0;
    int i;

#ifdef AT_PERSISTENT
    while (*s=='@') {
      persistent = 1;
      ++s;
    }
#endif
    sptr = s;
    kwd = findkeyword(parse_token(&sptr), lang);

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

    ord = (order*)malloc(sizeof(order));
    for (i=0;i!=nlocales;++i) {
      if (locale_array[i]==lang) break;
    }
    if (i==nlocales) locale_array[nlocales++] = lang;
    ord->_lindex = (unsigned char)i;
    ord->_str = NULL;
    ord->_persistent = persistent;
    ord->_refcount = 1;
    ord->next = NULL;

    ord->_keyword = kwd;
#ifdef SHORT_STRINGS
    if (ord->_keyword==NOKEYWORD) {
      ord->_str = strdup(s);
    } else {
      while (isspace(*(unsigned char*)sptr)) ++sptr;
      if (*sptr) {
        ord->_str = strdup(sptr);
      }
    }
#else
    ord->_str = strdup(s);
#endif
    return ord;
  }
}

boolean
is_exclusive(const order * ord)
{
  const struct locale * lang = locale_array[ord->_lindex];
  param_t param;

  switch (ord->_keyword) {
    case K_MOVE:
    case K_WEREWOLF:
      /* these should not become persistent */
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
    case K_ZUECHTE:
    case K_PIRACY:
      return true;

#if GROWING_TREES
    case K_PFLANZE:
      return true;
#endif

    case K_FOLLOW:
      /* FOLLOW is only a long order if we are following a ship. */
      parser_pushstate();
      init_tokens(ord);
      skip_token();
      param = getparam(lang);
      parser_popstate();

      if (param == P_SHIP) return true;
      break;

    case K_MAKE:
      /* Falls wir MACHE TEMP haben, ignorieren wir es. Alle anderen
       * Arten von MACHE zaehlen aber als neue defaults und werden
       * behandelt wie die anderen (deswegen kein break nach case
       * K_MAKE) - und in thisorder (der aktuelle 30-Tage Befehl)
       * abgespeichert). */
      parser_pushstate();
      init_tokens(ord); /* initialize token-parser */
      skip_token();
      param = getparam(lang);
      parser_popstate();

      if (param != P_TEMP) return true;
      break;
  }
  return false;
}

boolean
is_persistent(const order * ord)
{
  boolean persist = ord->_persistent!=0;
	switch (ord->_keyword) {
    case K_MOVE:
    case K_WEREWOLF:
    case NOKEYWORD:
      /* lang, aber niemals persistent! */
      return false;

    case K_KOMMENTAR:
    case K_LIEFERE:
      return true;
	}

	return persist || is_exclusive(ord);
}

char * 
write_order(const order * ord, const struct locale * lang, char * buffer, size_t size)
{
  if (ord==0 || ord->_keyword==NOKEYWORD) {
    buffer[0]=0;
  } else {
    char * s = getcommand(ord);
    strncpy(buffer, s, size);
    free(s);
  }
  return buffer;
}
