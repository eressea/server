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

keyword_t
get_keyword(const order * ord)
{
  if (ord==NULL) {
    return NOKEYWORD;
  }
  return ord->_keyword;
}

const char * 
getcommand(const order * ord)
{
  return ord->_str;
}

void 
free_order(order * ord)
{
	if (ord!=NULL && --ord->_refcount==0) {
		free(ord->_str);
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
  while (isspace(*s)) ++s;
  if (*s==0) return NULL;
  else {
	order * ord = (order*)malloc(sizeof(order));
	ord->_str = strdup(s);
	ord->_keyword = findkeyword(parse_token(&s), lang);
	ord->_refcount = 1;
	ord->next = NULL;
	return ord;
  }
}

boolean
is_persistent(const order * cmd)
{
#ifdef AT_PERSISTENT
	if (cmd->_str[0] == '@') return true;
#endif      /* Nur kurze Befehle! */
	switch (cmd->_keyword) {
	case K_KOMMENTAR:
	case K_LIEFERE:
		return true;
		break;
	}
	return false;
}

char * 
write_order(const order * cmd, const struct locale * lang, char * buffer, size_t size)
{
  if (cmd==0) {
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
