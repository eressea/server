#include <config.h>
#include <eressea.h>
#include "order.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void
append_order(orders * os, order * o)
{
	assert(o->next==NULL);
	if (os->end==NULL) os->list = o;
	else *(os->end) = o;
	os->end = &o->next;
}

order *
remove_order(orders * os, order * o)
{
	order ** op = &os->list;
	while (*op && o!=*op) op=&(*op)->next;
	assert(*op);
	if (o->next==NULL) os->end=op;
	*op = o->next;
	o->next = NULL;
	return o;
}

void
free_order(order * o)
{
	if (o->data.v) {
		char ** ca = (char**)o->data.v;
		while (*ca) free(*(ca++));
		free(o->data.v);
	}
	free(o);
}

#define MAXPARAM 128

#include "eressea.h"

order *
new_order(const char * cmd)
{
	const char * params[MAXPARAM+1];
	char ** cp;
	int i;
	order * o = calloc(sizeof(order), 1);
	o->type = igetkeyword(cmd);
	for (i=0;i!=MAXPARAM;++i) {
		const char * s = getstrtoken();
		if (s==NULL || *s==0) break;
		params[i] = strdup(s);
	}
	cp = malloc(sizeof(const char*)*(i+1));
	memcpy(cp, params, sizeof(const char*)*i);
	cp[i]=NULL;
	o->data.v = (void*) cp;
	return o;
}

const char *
order_string(const order * o, char * buf, size_t len)
{
	char * c;
	static char retval[4096];
	size_t slen;
	if (buf==NULL) {
		buf = retval;
		len=4096;
	}
	c = buf;
	strncpy(c, locale_string(u->faction->locale, keywords[o->type]), len);
	slen = strlen(c);
	len -= slen;
	c += slen;
	if (o->data.v) {
		char ** cp = o->data.v;
		while (*cp) {
			slen = strlen(*cp);
			*c = ' ';
			if (slen>len-2) {
				strncpy(c+1, *cp, len-1);
				break;
			}
			strncpy(c+1, *cp, len);
			len -= slen+1;
			c += slen+1;
		}
	}
	buf[len-1] = '\0';
	return buf;
}
