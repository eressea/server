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

#define SHORT_STRINGS
#define SHARE_ORDERS

typedef struct locale_data {
#ifdef SHARE_ORDERS
  struct order_data * short_orders[MAXKEYWORDS];
#endif
  const struct locale * lang;
} locale_data;

static struct locale_data * locale_array[16];
static int nlocales = 0;

typedef struct order_data {
  char * _str; 
  keyword_t _keyword;
  int _lindex : 7;
  int _refcount : 16;
} order_data;

static void
release_data(order_data * data)
{
  if (data) {
    if (--data->_refcount == 0) {
      if (data->_str) free(data->_str);
      free(data);
    }
  }
}

void
replace_order(order * dst, const order * src)
{
  release_data(dst->data);
  dst->data = src->data;
  ++src->data->_refcount;
  dst->_persistent = src->_persistent;
}

keyword_t
get_keyword(const order * ord)
{
  if (ord==NULL) {
    return NOKEYWORD;
  }
  return ord->data->_keyword;
}

char *
getcommand(const order * ord)
{
  char sbuffer[DISPLAYSIZE*2];
  char * str = sbuffer;
  
  assert(ord->data->_lindex<nlocales);
  if (ord->_persistent) *str++ = '@';
#ifdef SHORT_STRINGS
  if (ord->data->_keyword!=NOKEYWORD) {
    const struct locale * lang = locale_array[ord->data->_lindex]->lang;

    strcpy(str, LOC(lang, keywords[ord->data->_keyword]));
    str += strlen(str);
    if (ord->data->_str) {
      *str++ = ' ';
      *str = 0;
    }
  }
#endif
  if (ord->data->_str) strcpy(str, ord->data->_str);
  return strdup(sbuffer);
}

void 
free_order(order * ord)
{
  if (ord!=NULL) {
    release_data(ord->data);
    free(ord);
  }
}

order *
copy_order(order * src)
{
  if (src!=NULL) {
    order * ord = (order*)malloc(sizeof(order));
    ord->next = NULL;
    ord->_persistent = src->_persistent;
    ord->data = src->data;
    ++ord->data->_refcount;
    return ord;
  }
  return NULL;
}

void 
set_order(struct order ** destp, struct order * src)
{
  if (*destp==src) return;
  free_order(*destp);
  *destp = copy_order(src);
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
    while (isspace(*(unsigned char*)sptr)) ++sptr;

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

    for (i=0;i!=nlocales;++i) {
      if (locale_array[i]->lang==lang) break;
    }
    if (i==nlocales) {
      locale_array[nlocales] = (locale_data*)calloc(1, sizeof(locale_data));
      locale_array[nlocales]->lang = lang;
      ++nlocales;
    }

    ord = (order*)malloc(sizeof(order));
    ord->_persistent = persistent;
    ord->next = NULL;
    ord->data = NULL;

#ifdef SHARE_ORDERS
    if (kwd!=NOKEYWORD && *sptr == 0) {
      ord->data = locale_array[i]->short_orders[kwd];
      if (ord->data != NULL) {
        ++ord->data->_refcount;
      }
    }
#endif

    if (ord->data==NULL) {
      ord->data = (order_data*)malloc(sizeof(order_data));
      ord->data->_lindex = (unsigned char)i;
      ord->data->_str = NULL;
      ord->data->_refcount = 1;
      ord->data->_keyword = kwd;
#ifdef SHARE_ORDERS
      if (kwd!=NOKEYWORD && *sptr == 0) {
        locale_array[i]->short_orders[kwd] = ord->data;
        ++ord->data->_refcount;
      }
#endif
#ifdef SHORT_STRINGS
      if (ord->data->_keyword==NOKEYWORD) {
        ord->data->_str = strdup(s);
      } else {
        if (*sptr) {
          ord->data->_str = strdup(sptr);
        }
      }
#else
      ord->data->_str = strdup(s);
#endif
    }
    return ord;
  }
}

boolean
is_repeated(const order * ord)
{
  const struct locale * lang = locale_array[ord->data->_lindex]->lang;
  param_t param;

  switch (ord->data->_keyword) {
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
is_exclusive(const order * ord)
{
  const struct locale * lang = locale_array[ord->data->_lindex]->lang;
  param_t param;

  switch (ord->data->_keyword) {
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
	switch (ord->data->_keyword) {
    case K_MOVE:
    case K_WEREWOLF:
    case NOKEYWORD:
      /* lang, aber niemals persistent! */
      return false;

    case K_KOMMENTAR:
    case K_LIEFERE:
      return true;
	}

	return persist || is_repeated(ord);
}

char * 
write_order(const order * ord, const struct locale * lang, char * buffer, size_t size)
{
  if (ord==0 || ord->data->_keyword==NOKEYWORD) {
    buffer[0]=0;
  } else {
    char * s = getcommand(ord);
    strncpy(buffer, s, size);
    free(s);
  }
  return buffer;
}
