/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 
 */
#include <config.h>
#include "command.h"

#include "umlaut.h"
#include "language.h"

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct command {
	parser fun;
	struct tnode * nodes;
} command;

tnode *
stree_find(const syntaxtree * stree, const struct locale * lang)
{
	while (stree) {
		if (stree->lang==lang) return stree->root;
		stree = stree->next;
	}
	return NULL;
}

syntaxtree *
stree_create(void)
{
	syntaxtree * sroot = NULL;
	const struct locale * lang = locales;
	while (lang) {
		syntaxtree * stree = malloc(sizeof(syntaxtree));
		stree->lang = lang;
		stree->next = sroot;
		sroot=stree;
		lang=nextlocale(lang);
	}
	return sroot;
}

void
add_command(struct tnode * keys, struct tnode * tnext, 
				const char * str, parser fun)
{
	command * cmd = malloc(sizeof(command));
	cmd->fun = fun;
	cmd->nodes = tnext;
	addtoken(keys, str, (void*)cmd);
}

static void
do_command_i(const struct tnode * keys, void * u, const char * str, struct order * ord)
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
		if (cmd->nodes) {
			assert(!cmd->fun);
			do_command_i(cmd->nodes, u, ++c, ord);
			return;
		}
		assert(cmd->fun);
		cmd->fun(cmd->nodes, ++c, u, ord);
	}
}

extern char * getcommand(struct order * ord);

void
do_command(const struct tnode * keys, void * u, struct order * ord)
{
  char * cmd = getcommand(ord);
  do_command_i(keys, u, cmd, ord);
  free(cmd);
}
