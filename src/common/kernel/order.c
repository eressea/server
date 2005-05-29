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
#include "skill.h"

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define SHORT_STRINGS

#ifdef SHARE_ORDERS
# define ORD_KEYWORD(ord) (ord)->data->_keyword
# define ORD_LOCALE(ord) locale_array[(ord)->data->_lindex]->lang
# define ORD_STRING(ord) (ord)->data->_str
#else
# define ORD_KEYWORD(ord) (ord)->data._keyword
# define ORD_LOCALE(ord) locale_array[ord->data._lindex]->lang
# define ORD_STRING(ord) (ord)->data._str
#endif


typedef struct locale_data {
#ifdef SHARE_ORDERS
  struct order_data * short_orders[MAXKEYWORDS];
  struct order_data * study_orders[MAXSKILLS];
#endif
  const struct locale * lang;
} locale_data;

static struct locale_data * locale_array[16];
static int nlocales = 0;

#ifdef SHARE_ORDERS
typedef struct order_data {
  char * _str; 
  int _refcount : 20;
  int _lindex : 4;
  keyword_t _keyword;
} order_data;
#endif

#ifdef SHARE_ORDERS
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
#endif

void
replace_order(order ** dlist, order * orig, const order * src)
{
  while (*dlist!=NULL) {
    order * dst = *dlist;
    if (dst->data==orig->data) {
      order * cpy = copy_order(src);
      *dlist = cpy;
      cpy->next = dst->next;
      dst->next = 0;
      free_order(dst);
    }
    dlist = &(*dlist)->next;
  }
}

keyword_t
get_keyword(const order * ord)
{
  if (ord==NULL) {
    return NOKEYWORD;
  }
  return ORD_KEYWORD(ord);
}

char *
getcommand(const order * ord)
{
  char sbuffer[DISPLAYSIZE*2];
  char * str = sbuffer;
  const char * text = ORD_STRING(ord);
#ifdef SHORT_STRINGS
  keyword_t kwd = ORD_KEYWORD(ord);
#endif

  if (ord->_persistent) *str++ = '@';
#ifdef SHORT_STRINGS
  if (kwd!=NOKEYWORD) {
    const struct locale * lang = ORD_LOCALE(ord);
    strcpy(str, LOC(lang, keywords[kwd]));
    str += strlen(str);
    if (text) {
      *str++ = ' ';
      *str = 0;
    }
  }
#endif
  if (text) strcpy(str, text);
  return strdup(sbuffer);
}

void 
free_order(order * ord)
{
  if (ord!=NULL) {
    assert(ord->next==0);

#ifdef SHARE_ORDERS
    release_data(ord->data);
#else
    if (ord->data._str) free(ord->data._str);
#endif
    free(ord);
  }
}

order *
copy_order(const order * src)
{
  if (src!=NULL) {
    order * ord = (order*)malloc(sizeof(order));
    ord->next = NULL;
    ord->_persistent = src->_persistent;
    ord->data = src->data;
#ifdef SHARE_ORDERS
    ++ord->data->_refcount;
#endif
    return ord;
  }
  return NULL;
}

void 
set_order(struct order ** destp, struct order * src)
{
  if (*destp==src) return;
  free_order(*destp);
  *destp = src;
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

#ifdef SHARE_ORDERS
static order_data *
create_data(keyword_t kwd, const char * s, const char * sptr, int lindex)
{
  order_data * data;
  const struct locale * lang = locale_array[lindex]->lang;

  /* learning, only one order_data per skill required */
  if (kwd==K_STUDY) {
    skill_t sk = findskill(parse_token(&sptr), lang);
    switch (sk) {
      case NOSKILL: /* fehler */
        break;
      case SK_MAGIC: /* kann parameter haben */
        if (*sptr != 0) break;
      default: /* nur skill als Parameter, keine extras */
        data = locale_array[lindex]->study_orders[sk];
        if (data==NULL) {
          data = (order_data*)malloc(sizeof(order_data));
          locale_array[lindex]->study_orders[sk] = data;
          data->_keyword = kwd;
          data->_lindex = lindex;
#ifdef SHORT_STRINGS
          data->_str = strdup(skillname(sk, lang));
#else
          sprintf(buf, "%s %s", LOC(lang, keywords[kwd]), skillname(sk, lang));
          data->_str = strdup(buf);
#endif
          data->_refcount = 1;
        }
        ++data->_refcount;
        return data;
    }
  }

  /* orders with no parameter, only one order_data per order required */
  else if (kwd!=NOKEYWORD && *sptr == 0) {
    data = locale_array[lindex]->short_orders[kwd];
    if (data == NULL) {
      data = (order_data*)malloc(sizeof(order_data));
      locale_array[lindex]->short_orders[kwd] = data;
      data->_keyword = kwd;
      data->_lindex = lindex;
#ifdef SHORT_STRINGS
      data->_str = NULL;
#else
      data->_str = strdup(LOC(lang, keywords[kwd]));
#endif
      data->_refcount = 1;
    }
    ++data->_refcount;
    return data;
  }
  data = (order_data*)malloc(sizeof(order_data));
  data->_keyword = kwd;
  data->_lindex = lindex;
  data->_str = s?strdup(s):NULL;
  data->_refcount = 1;
  return data;
}
#endif

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
    int lindex;

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

    for (lindex=0;lindex!=nlocales;++lindex) {
      if (locale_array[lindex]->lang==lang) break;
    }
    if (lindex==nlocales) {
      locale_array[nlocales] = (locale_data*)calloc(1, sizeof(locale_data));
      locale_array[nlocales]->lang = lang;
      ++nlocales;
    }

    ord = (order*)malloc(sizeof(order));
    ord->_persistent = persistent;
    ord->next = NULL;

#ifdef SHORT_STRINGS
    if (kwd!=NOKEYWORD) s = (*sptr)?sptr:NULL;
#endif
#ifdef SHARE_ORDERS
    ord->data = create_data(kwd, s, sptr, lindex);
#else
    ord->data._keyword = kwd;
    ord->data._lindex = lindex;
    ord->data._str = s?strdup(s):NULL;
#endif

    return ord;
  }
}

boolean
is_repeated(const order * ord)
{
  keyword_t kwd = ORD_KEYWORD(ord);
  const struct locale * lang = ORD_LOCALE(ord);
  param_t param;

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
    case K_BREED:
    case K_PIRACY:
      return true;

#if GROWING_TREES
    case K_PLANT:
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
  keyword_t kwd = ORD_KEYWORD(ord);
  const struct locale * lang = ORD_LOCALE(ord);
  param_t param;

  switch (kwd) {
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
    case K_BREED:
    case K_PIRACY:
      return true;

#if GROWING_TREES
    case K_PLANT:
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
  keyword_t kwd = ORD_KEYWORD(ord);
  boolean persist = ord->_persistent!=0;
	switch (kwd) {
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
  if (ord==0) {
    buffer[0]=0;
  } else {
    keyword_t kwd = ORD_KEYWORD(ord);
    if (kwd==NOKEYWORD) {
      buffer[0]=0;
    } else {
      char * s = getcommand(ord);
      strncpy(buffer, s, size);
      free(s);
    }
  }
  return buffer;
}
