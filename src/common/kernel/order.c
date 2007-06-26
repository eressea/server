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

#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/language.h>
#include <util/parser.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

# define ORD_KEYWORD(ord) (ord)->data->_keyword
# define ORD_LOCALE(ord) locale_array[(ord)->data->_lindex]->lang
# define ORD_STRING(ord) (ord)->data->_str


typedef struct locale_data {
  struct order_data * short_orders[MAXKEYWORDS];
  struct order_data * study_orders[MAXSKILLS];
  const struct locale * lang;
} locale_data;

static struct locale_data * locale_array[16];
static int nlocales = 0;

typedef struct order_data {
  xmlChar * _str; 
  int _refcount : 20;
  int _lindex : 4;
  keyword_t _keyword;
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

static xmlChar *
get_command(const order * ord, xmlChar * sbuffer, size_t bufsize)
{
  xmlChar * str = sbuffer;
  const xmlChar * text = ORD_STRING(ord);
  keyword_t kwd = ORD_KEYWORD(ord);

  if (ord->_persistent) *str++ = '@';
  if (kwd!=NOKEYWORD) {
    const struct locale * lang = ORD_LOCALE(ord);
    size_t size = bufsize-(str-sbuffer);
    if (text) --size;
    str += strlcpy((char*)str, (const char*)LOC(lang, keywords[kwd]), size);
    if (text) {
      *str++ = ' ';
    }
  }
  if (text) {
    str += strlcpy((char*)str, (const char *)text, bufsize-(str-sbuffer));
  }
  return sbuffer;
}

xmlChar *
getcommand(const order * ord)
{
  xmlChar sbuffer[DISPLAYSIZE*2];
  return xmlStrdup(get_command(ord, sbuffer, sizeof(sbuffer)));
}

void 
free_order(order * ord)
{
  if (ord!=NULL) {
    assert(ord->next==0);

    release_data(ord->data);
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

static order_data *
create_data(keyword_t kwd, const xmlChar * sptr, int lindex)
{
  const xmlChar * s = sptr;
  order_data * data;
  const struct locale * lang = locale_array[lindex]->lang;

  if (kwd!=NOKEYWORD) s = (*sptr)?sptr:NULL;

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
          const xmlChar * skname = skillname(sk, lang);
          data = (order_data*)malloc(sizeof(order_data));
          locale_array[lindex]->study_orders[sk] = data;
          data->_keyword = kwd;
          data->_lindex = lindex;
          if (xmlStrchr(skname, ' ')!=NULL) {
            size_t len = xmlStrlen(skname);
            data->_str = (xmlChar*)malloc(len+3);
            data->_str[0]='\"';
            memcpy(data->_str+1, skname, len);
            data->_str[len+1]='\"';
            data->_str[len+2]='\0';
          } else {
            data->_str = xmlStrdup(skname);
          }
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
      data->_str = NULL;
      data->_refcount = 1;
    }
    ++data->_refcount;
    return data;
  }
  data = (order_data*)malloc(sizeof(order_data));
  data->_keyword = kwd;
  data->_lindex = lindex;
  data->_str = s?xmlStrdup(s):NULL;
  data->_refcount = 1;
  return data;
}

static order *
create_order_i(keyword_t kwd, const xmlChar * sptr, int persistent, const struct locale * lang)
{
  order * ord = NULL;
  int lindex;

  /* if this is just nonsense, then we skip it. */
  if (lomem) {
    switch (kwd) {
        case K_KOMMENTAR:
        case NOKEYWORD:
          return NULL;
        case K_LIEFERE:
          kwd = K_GIVE;
          persistent = 1;
          break;
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

  ord->data = create_data(kwd, sptr, lindex);

  return ord;
}

order *
create_order(keyword_t kwd, const struct locale * lang, const char * params, ...)
{
  va_list marker;
  char zBuffer[DISPLAYSIZE];
  char * sptr = zBuffer;
  char quote = 0;

  va_start(marker, params);
  while (params) {
    switch (*params) {
      case '%':
      case ' ':
        /* ignore these, they are syntactical sugar */
        break;
      case '"':
      case '\'':
        *sptr++ = *params;
        if (!quote) quote = *params;
        else if (quote==*params) {
          quote = 0;
          *sptr++ = ' ';
        }
        break;
      case 's':
        sptr += strlcpy(sptr, va_arg(marker, const char *), sizeof(zBuffer)-(sptr-zBuffer));
        if (!quote) *sptr++ = ' ';
        break;
      case 'd':
        sptr += strlcpy(sptr, itoa10(va_arg(marker, int)), sizeof(zBuffer)-(sptr-zBuffer));
        break;
      case 'i':
        sptr += strlcpy(sptr, itoa36(va_arg(marker, int)), sizeof(zBuffer)-(sptr-zBuffer));
        break;
    }
  }
  va_end(marker);

  return create_order_i(kwd, (const xmlChar*)sptr, 0, lang);
}

order * 
parse_order(const xmlChar * s, const struct locale * lang)
{
  while (*s && !isalnum(*(unsigned char*)s) && !ispunct(*(unsigned char*)s)) ++s;
  if (*s!=0) {
    keyword_t kwd;
    const xmlChar * sptr;
    int persistent = 0;

    while (*s=='@') {
      persistent = 1;
      ++s;
    }
    sptr = s;
    kwd = findkeyword(parse_token(&sptr), lang);
    if (kwd!=NOKEYWORD) {
      while (isspace(*(unsigned char*)sptr)) ++sptr;
      s = sptr;
    }
    return create_order_i(kwd, s, persistent, lang);
  }
  return NULL;
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

    case K_PLANT:
      return true;

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

    case K_PLANT:
      return true;

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

xmlChar * 
write_order(const order * ord, const struct locale * lang, xmlChar * buffer, size_t size)
{
  if (ord==0) {
    buffer[0]=0;
  } else {
    keyword_t kwd = ORD_KEYWORD(ord);
    if (kwd==NOKEYWORD) {
      const xmlChar * text = ORD_STRING(ord);
      strlcpy((char *)buffer, (const char *)text, size);
    } else {
      get_command(ord, buffer, size);
    }
  }
  return buffer;
}
