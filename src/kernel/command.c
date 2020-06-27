#include <platform.h>
#include <kernel/config.h>
#include "command.h"

#include <kernel/faction.h>
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
    struct command *next;
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

void stree_free(syntaxtree *stree) {
    while (stree->cmds) {
        command *next = stree->cmds->next;
        free(stree->cmds);
        stree->cmds = next;
    }
    while (stree) {
        syntaxtree *snext = stree->next;
        freetokens(stree->root);
        free(stree);
        stree = snext;
    }
}

syntaxtree *stree_create(void)
{
    syntaxtree *sroot = NULL;
    const struct locale *lang = locales;
    while (lang) {
        syntaxtree *stree = (syntaxtree *)malloc(sizeof(syntaxtree));
        if (!stree) abort();
        stree->lang = lang;
        stree->next = sroot;
        stree->root = 0;
        stree->cmds = 0;
        sroot = stree;
        lang = nextlocale(lang);
    }
    return sroot;
}

void stree_add(struct syntaxtree *stree, const char *str, parser fun) {
    command *cmd = (command *)malloc(sizeof(command));
    variant var;

    assert(str);
    if (!cmd) abort();
    cmd->fun = fun;
    var.v = cmd;
    cmd->next = stree->cmds;
    stree->cmds = cmd;
    addtoken(&stree->root, str, var);
}

static int do_command_i(const struct tnode *keys, struct unit *u, struct order *ord)
{
    char token[128];
    const char *c;
    variant var;

    c = gettoken(token, sizeof(token));
    if (findtoken(keys, c, &var) == E_TOK_SUCCESS) {
        command *cmd = (command *)var.v;
        if (cmd->fun) {
            cmd->fun(0, u, ord);
            return E_TOK_SUCCESS;
        }
    }
    return E_TOK_NOMATCH;
}

void do_command(const struct tnode *keys, struct unit *u, struct order *ord)
{
    init_order_depr(ord);
    if (do_command_i(keys, u, ord) != E_TOK_SUCCESS) {
        char cmd[ORDERSIZE];
        get_command(ord, u->faction->locale, cmd, sizeof(cmd));
        log_warning("%s failed command '%s'\n", unitname(u), cmd);
    }
}
