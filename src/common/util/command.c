/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 
 */
#include <config.h>

#include "command.h"

#include "umlaut.h"

/* libc includes */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct command {
	struct command * next;
	const char * key;
	void (*perform)(const char *, void *, const char *);
} command;

void
add_command(struct tnode * keys, command ** cmds, const char * str, void(*fun)(const char*, void *, const char*))
{
	command * nc = calloc(sizeof(command), 1);
	nc->key = str;
	nc->perform = fun;
	nc->next = *cmds;
	*cmds = nc;

	addtoken(keys, str, (void*)nc);
}

void
do_command(const struct tnode * keys, void * u, const char * str)
{
	int i;
	char zText[16];
	const char * c;
	command * cm;

	while (isspace(*str)) ++str;
	c = str;
	while (isalnum(*c)) ++c;
	i = min(16, c-str);
	strncpy(zText, str, i);
	zText[i]=0;
	if (findtoken(keys, zText, (void**)&cm)==E_TOK_SUCCESS && cm->perform) cm->perform(++c, u, str);
}

