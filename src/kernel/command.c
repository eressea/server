/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.

 */
#include <platform.h>
#include <kernel/config.h>
#include "command.h"

#include <kernel/order.h>
#include <kernel/unit.h>

#include <util/umlaut.h>
#include <util/language.h>
#include <util/log.h>
#include <util/parser.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct command {
    parser fun;
    void *nodes;
} command;

void *stree_find(const syntaxtree * stree, const struct locale *lang)
{
    while (stree) {
        if (stree->lang == lang)
            return stree->root;
        stree = stree->next;
    }
    return NULL;
}

syntaxtree *stree_create(void)
{
    syntaxtree *sroot = NULL;
    const struct locale *lang = locales;
    while (lang) {
        syntaxtree *stree = (syntaxtree *)malloc(sizeof(syntaxtree));
        stree->lang = lang;
        stree->next = sroot;
        stree->root = 0;
        sroot = stree;
        lang = nextlocale(lang);
    }
    return sroot;
}

void
add_command(void **keys, void *tnext,
const char *str, parser fun)
{
    command *cmd = (command *)malloc(sizeof(command));
    variant var;

    cmd->fun = fun;
    cmd->nodes = tnext;
    var.v = cmd;
    addtoken(keys, str, var);
}

static int do_command_i(const void *keys, struct unit *u, struct order *ord)
{
    char token[128];
    const char *c;
    variant var;

    c = gettoken(token, sizeof(token));
    if (findtoken(keys, c, &var) == E_TOK_SUCCESS) {
        command *cmd = (command *)var.v;
        if (cmd->nodes && *c) {
            assert(!cmd->fun);
            return do_command_i(cmd->nodes, u, ord);
        }
        else if (cmd->fun) {
            cmd->fun(cmd->nodes, u, ord);
            return E_TOK_SUCCESS;
        }
    }
    return E_TOK_NOMATCH;
}

void do_command(const void *keys, struct unit *u, struct order *ord)
{
    init_order(ord);
    if (do_command_i(keys, u, ord) != E_TOK_SUCCESS) {
        char cmd[ORDERSIZE];
        get_command(ord, cmd, sizeof(cmd));
        log_warning("%s failed command '%s'\n", unitname(u), cmd);
    }
}
