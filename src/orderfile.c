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
#include "util/param.h"
#include "util/parser.h"
#include "util/password.h"
#include "util/order_parser.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static void begin_orders(unit *u) {
    if (!u->defaults) {
        u->defaults = u->orders;
    }
    else {
        free_orders(&u->orders);
    }
    u->orders = NULL;
}

static void handle_faction(void *userData, int no, const char *password) {
    parser_state *state = (parser_state *)userData;
    faction * f = findfaction(no);
    if (!f) {
        log_debug("orders for unknown faction %s", itoa36(no));
    }
    else {
        state->u = NULL;
        state->next_order = NULL;
        state->f = NULL;
        if (checkpasswd(f, password)) {
            state->f = f;
            f->lastorders = turn;
            f->flags &= ~(FFL_PAUSED);
        }
        else {
            log_debug("invalid password for faction %s", itoa36(no));
            ADDMSG(&f->msgs, msg_message("wrongpasswd", "password", password));
        }
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
    const char * tok, *input;
    char buffer[64];
    const struct locale *lang;
    faction * f = state->f;

    lang = f ? f->locale : default_locale;
    ltrim(&str);
    if (*str == 0) return;
    input = str;
    tok = parse_token(&input, buffer, sizeof(buffer));
    if (tok) {
        param_t p = findparam(tok, lang);
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
            unit *u = state->u;
            order *ord = parse_order(str, lang);
            if (ord) {
                *state->next_order = ord;
                state->next_order = &ord->next;
            }
            else {
                ADDMSG(&u->faction->msgs, msg_message("parse_error", "unit command", u, str));
            }
        }
    }
}

OP_Parser parser_create(parser_state* state)
{
    OP_Parser parser;
    parser = OP_ParserCreate();
    if (parser) {
        OP_SetOrderHandler(parser, handle_order);
        OP_SetUserData(parser, state);
    }
    return parser;
}

int parser_parse(OP_Parser parser, const char* input, size_t len, bool done)
{
    if (OP_Parse(parser, input, len, done) == OP_STATUS_ERROR) {
        /* TODO: error message */
        return (int) OP_GetErrorCode(parser);
    }
    return (int) OP_ERROR_NONE;
}

void parser_free(OP_Parser parser)
{
    OP_ParserFree(parser);
}

int parseorders(FILE *F)
{
    char buf[4096];
    int done = 0, err = 0;
    OP_Parser parser;
    parser_state state = { NULL, NULL };

    parser = parser_create(&state);
    if (!parser) {
        /* TODO: error message */
        return errno;
    }

    while (!done && err == 0) {
        size_t len = fread(buf, 1, sizeof(buf), F);
        if (ferror(F)) {
            /* TODO: error message */
            err = errno;
            break;
        }
        done = feof(F);
        err = parser_parse(parser, buf, len, done);
    }
    parser_free(parser);
    return err;
}
