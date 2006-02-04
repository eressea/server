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
#include "log.h"

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
		syntaxtree * stree = (syntaxtree *)malloc(sizeof(syntaxtree));
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
	command * cmd = (command *)malloc(sizeof(command));
  variant var;

  cmd->fun = fun;
	cmd->nodes = tnext;
  var.v = cmd;
	addtoken(keys, str, var);
}

static int
do_command_i(const struct tnode * keys, void * u, const char * str, struct order * ord)
{
  size_t i;
  char zText[16];
  const char * c;
  variant var;

  while (isspace(*str)) ++str;
  c = str;
  while (isalnum(*c)) ++c;
  i = min(16, c-str);
  strncpy(zText, str, i);
  zText[i]=0;
  if (findtoken(keys, zText, &var)==E_TOK_SUCCESS) {
    command * cmd = (command *)var.v;
    if (cmd->nodes && *c) {
      assert(!cmd->fun);
      return do_command_i(cmd->nodes, u, ++c, ord);
    } else if (cmd->fun) {
      cmd->fun(cmd->nodes, ++c, u, ord);
      return E_TOK_SUCCESS;
    }
  }
  return E_TOK_NOMATCH;
}

struct unit;
struct order;
extern char * getcommand(struct order * ord);
extern char * unitname(struct unit * u);

void
do_command(const struct tnode * keys, void * u, struct order * ord)
{
  char * cmd = getcommand(ord);
  if (do_command_i(keys, u, cmd, ord)!=E_TOK_SUCCESS) {
    log_warning(("%s failed GM command '%s'\n", unitname(u), cmd));
  }
  free(cmd);
}
