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
	void(*fun)(const char*, void *, const char*);
} command;

void
add_command(struct tnode * keys, const char * str, void(*fun)(const char*, void *, const char*))
{
	command * cmd = malloc(sizeof(command));
	cmd->fun = fun;
	addtoken(keys, str, (void*)cmd);
}

int
do_command(const struct tnode * keys, void * u, const char * str)
{
	int i;
	char zText[16];
	const char * c;
	command * cmd;

	while (isspace(*str)) ++str;
	c = str;
	while (isalnum(*c)) ++c;
	i = min(16, c-str);
	strncpy(zText, str, i);
	zText[i]=0;
	if (findtoken(keys, zText, (void**)&cmd)==E_TOK_SUCCESS) {
		cmd->fun(++c, u, str);
		return 0;
	}
	return 1;
}
