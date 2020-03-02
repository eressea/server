#include <platform.h>
#include "orderfile.h"

#include "kernel/calendar.h"
#include "kernel/faction.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/unit.h"

#include "util/base36.h"
#include "util/message.h"
#include "util/language.h"
#include "util/log.h"
#include "util/filereader.h"
#include "util/param.h"
#include "util/parser.h"
#include "util/password.h"
#include "util/order_parser.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static void begin_orders(unit *u) {
    if ((u->flags & UFL_ORDERS) == 0) {
        order **ordp;
        /* alle wiederholbaren, langen befehle werden gesichert: */
        u->flags |= UFL_ORDERS;
        u->old_orders = u->orders;
        ordp = &u->old_orders;
        while (*ordp) {
            order *ord = *ordp;
            keyword_t kwd = getkeyword(ord);
            if (!is_repeated(kwd)) {
                *ordp = ord->next;
                ord->next = NULL;
                free_order(ord);
            }
            else {
                ordp = &ord->next;
            }
        }
    }
    else {
        free_orders(&u->orders);
    }
    u->orders = NULL;
}

typedef struct parser_state {
    unit *u;
    faction *f;
    order **next_order;
} parser_state;

static void handle_faction(void *userData, int no, const char *password) {
    parser_state *state = (parser_state *)userData;
    faction * f = state->f = findfaction(no);
    if (!f) {
        log_debug("orders for unknown faction %s", itoa36(no));
    }
    else {
        if (checkpasswd(f, password)) {
            f->lastorders = turn;
        }
        else {
            log_debug("invalid password for faction %s", itoa36(no));
            ADDMSG(&f->msgs, msg_message("wrongpasswd", "password", password));
        }
        state->u = NULL;
        state->next_order = NULL;
    }
}

static void handle_unit(void *userData, int no) {
    parser_state *state = (parser_state *)userData;
    unit * u = findunit(no);

    state->u = NULL;
    if (!u) {
        /* TODO: error message */
    }
    else if (u->faction != state->f) {
        /* TODO: error message */
    }
    else {
        state->u = u;
        begin_orders(u);
        state->next_order = &u->orders;
    }
}

static void handle_order(void *userData, const char *str) {
    parser_state *state = (parser_state *)userData;
    const char * tok, *input = str;
    char buffer[64];
    const struct locale *lang;
    param_t p;
    faction * f = state->f;

    lang = f ? f->locale : default_locale;
    tok = parse_token(&input, buffer, sizeof(buffer));
    p = findparam(tok, lang);
    if (p == P_FACTION || p == P_GAMENAME) {
        tok = parse_token(&input, buffer, sizeof(buffer));
        if (tok) {
            int no = atoi36(tok);
            tok = parse_token(&input, buffer, sizeof(buffer));
            handle_faction(userData, no, tok);
        }
        else {
            /* TODO: log_error() */
        }
    }
    else if (p == P_UNIT) {
        tok = parse_token(&input, buffer, sizeof(buffer));
        if (tok) {
            int no = atoi36(tok);
            handle_unit(userData, no);
        }
    }
    else if (p == P_NEXT) {
        state->f = NULL;
        state->u = NULL;
        state->next_order = NULL;
    }
    else if (p == P_REGION) {
        state->u = NULL;
        state->next_order = NULL;
    }
    else if (state->u) {
        unit * u = state->u;
        order * ord = parse_order(str, lang);
        if (ord) {
            *state->next_order = ord;
            state->next_order = &ord->next;
        }
        else {
            ADDMSG(&u->faction->msgs, msg_message("parse_error", "unit command", u, str));
        }
    }
}

int parseorders(FILE *F)
{
    char buf[4096];
    int done = 0, err = 0;
    OP_Parser parser;
    parser_state state = { NULL, NULL };

    parser = OP_ParserCreate();
    if (!parser) {
        /* TODO: error message */
        return errno;
    }
    OP_SetOrderHandler(parser, handle_order);
    OP_SetUserData(parser, &state);

    while (!done) {
        size_t len = (int)fread(buf, 1, sizeof(buf), F);
        if (ferror(F)) {
            /* TODO: error message */
            err = errno;
            break;
        }
        done = feof(F);
        if (OP_Parse(parser, buf, len, done) == OP_STATUS_ERROR) {
            /* TODO: error message */
            err = (int)OP_GetErrorCode(parser);
            break;
        }
    }
    OP_ParserFree(parser);
    return err;
}
