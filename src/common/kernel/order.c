#include <config.h>
#include "eressea.h"

#include "order.h"

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

keyword_t
getkeyword(const order * ord)
{
	if (ord==NULL) {
		return NOKEYWORD;
	}
	igetstrtoken(ord->str);
	return ord->keyword;
}

const char * 
getcommand(const order * ord)
{
	return ord->str;
}

void 
free_order(order * ord)
{
	if (ord!=NULL && --ord->refcount==0) {
		free(ord->str);
		free(ord);
	}
}

order *
copy_order(order * ord)
{
	if (ord!=NULL) ++ord->refcount;
	return ord;
}

void 
set_order(struct order ** destp, struct order * src)
{
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
	order * ord = (order*)malloc(sizeof(order));
	ord->keyword = igetkeyword(s, lang);
	ord->refcount = 1;
	ord->str = strdup(s);
	ord->next = NULL;
	return ord;
}

boolean
is_persistent(const order * cmd)
{
#ifdef AT_PERSISTENT
	if (cmd->str[0] == '@') return true;
#endif      /* Nur kurze Befehle! */
	switch (cmd->keyword) {
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
	assert(igetkeyword(cmd->str, lang)==cmd->keyword);
	return strncpy(buffer, cmd->str, size);
}
