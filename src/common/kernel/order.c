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
		if (ord->_str!=NULL) free(ord->_str);
		free(ord);
	}
}

order *
copy_order(order * ord)
{
	if (ord!=NULL) ++ord->_refcount;
	return ord;
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
		free_order(ord);
	}
}

order * 
parse_order(const char * s, const struct locale * lang)
{
  while (isspace(*(unsigned char*)s)) ++s;
  if (*s==0) return NULL;
  else {
    const char * sptr;
    order * ord = (order*)malloc(sizeof(order));
    int i;

    for (i=0;i!=nlocales;++i) {
      if (locale_array[i]==lang) break;
    }
    if (i==nlocales) locale_array[nlocales++] = lang;
    ord->_lindex = (unsigned char)i;
    ord->_str = NULL;
    ord->_persistent = 0;
    ord->_refcount = 1;
    ord->next = NULL;

#ifdef AT_PERSISTENT
    if (*s=='@') {
      ord->_persistent = 1;
#ifdef SHORT_STRINGS
      ++s;
#endif
    }
#endif
    sptr = s;
    ord->_keyword = findkeyword(parse_token(&sptr), lang);
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
is_persistent(const order * cmd)
{
	switch (cmd->_keyword) {
    case NOKEYWORD:
      return false;
    case K_KOMMENTAR:
    case K_LIEFERE:
      return true;
	}
#ifdef AT_PERSISTENT
  if (cmd->_persistent) return true;
#endif      /* Nur kurze Befehle! */
	return false;
}

char * 
write_order(const order * ord, const struct locale * lang, char * buffer, size_t size)
{
  if (ord==0 || ord->_keyword==NOKEYWORD) {
    buffer[0]=0;
  } else {
#ifndef NDEBUG
    const char * s = cmd->_str;
    assert(findkeyword(parse_token(&s), lang)==cmd->_keyword);
#endif
    strncpy(buffer, cmd->_str, size);
  }
  return buffer;
}
