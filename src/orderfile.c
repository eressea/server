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
#include "util/order_parser.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static void begin_orders(unit *u) {
    if (u->flags & UFL_ORDERS) {
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

static unit *unitorders(input *in, faction *f)
{
    int i;
    unit *u;

    if (!f)
        return NULL;

    i = getid();
    u = findunit(i);

    if (u && u->faction == f) {
        order **ordp;

        begin_orders(u);
        ordp = &u->orders;

        for (;;) {
            const char *s;
            /* Erst wenn wir sicher sind, dass kein Befehl
            * eingegeben wurde, checken wir, ob nun eine neue
            * Einheit oder ein neuer Spieler drankommt */

            s = in->getbuf(in->data);
            if (s == NULL)
                break;

            if (s[0]) {
                if (s[0] != '@') {
                    char token[64];
                    const char *stok = s;
                    stok = parse_token(&stok, token, sizeof(token));

                    if (stok) {
                        bool quit = false;
                        param_t param = findparam(stok, u->faction->locale);
                        switch (param) {
                        case P_UNIT:
                        case P_REGION:
                            quit = true;
                            break;
                        case P_FACTION:
                        case P_NEXT:
                        case P_GAMENAME:
                            /* these terminate the orders, so we apply extra checking */
                            if (strlen(stok) >= 3) {
                                quit = true;
                                break;
                            }
                            else {
                                quit = false;
                            }
                            break;
                        default:
                            break;
                        }
                        if (quit) {
                            break;
                        }
                    }
                }
                /* Nun wird der Befehl erzeut und eingeh�ngt */
                *ordp = parse_order(s, u->faction->locale);
                if (*ordp) {
                    ordp = &(*ordp)->next;
                }
                else {
                    ADDMSG(&f->msgs, msg_message("parse_error", "unit command", u, s));
                }
            }
        }

    }
    else {
        return NULL;
    }
    return u;
}

static faction *factionorders(void)
{
    int fid = getid();
    faction *f = findfaction(fid);

    if (f != NULL && (f->flags & FFL_NPC) == 0) {
        char token[128];
        const char *pass = gettoken(token, sizeof(token));

        if (!checkpasswd(f, (const char *)pass)) {
            log_debug("Invalid password for faction %s", itoa36(fid));
            ADDMSG(&f->msgs, msg_message("wrongpasswd", "password", pass));
            return 0;
        }
        /* Die Partei hat sich zumindest gemeldet, so dass sie noch
        * nicht als unt�tig gilt */
        f->lastorders = turn;

    }
    else {
        log_debug("orders for invalid faction %s", itoa36(fid));
    }
    return f;
}

int read_orders(input *in)
{
    const char *b;
    int nfactions = 0;
    struct faction *f = NULL;
    const struct locale *lang = default_locale;

    /* TODO: recognize UTF8 BOM */
    b = in->getbuf(in->data);

    /* Auffinden der ersten Partei, und danach abarbeiten bis zur letzten
    * Partei */

    while (b) {
        char token[128];
        param_t p;
        const char *s;
        init_tokens_str(b);
        s = gettoken(token, sizeof(token));
        p = findparam_block(s, lang, true);
        switch (p) {
        case P_GAMENAME:
        case P_FACTION:
            f = factionorders();
            if (f) {
                lang = f->locale;
                ++nfactions;
            }

            b = in->getbuf(in->data);
            break;

            /* in factionorders wird nur eine zeile gelesen:
            * diejenige mit dem passwort. Die befehle der units
            * werden geloescht, und die Partei wird als aktiv
            * vermerkt. */

        case P_UNIT:
            if (!f || !unitorders(in, f)) {
                do {
                    b = in->getbuf(in->data);
                    if (!b) {
                        break;
                    }
                    init_tokens_str(b);
                    s = gettoken(token, sizeof(token));
                    p = (s && s[0] != '@') ? findparam(s, lang) : NOPARAM;
                } while ((p != P_UNIT || !f) && p != P_FACTION && p != P_NEXT
                    && p != P_GAMENAME);
            }
            break;

            /* Falls in unitorders() abgebrochen wird, steht dort entweder eine neue
            * Partei, eine neue Einheit oder das File-Ende. Das switch() wird erneut
            * durchlaufen, und die entsprechende Funktion aufgerufen. Man darf buf
            * auf alle F�lle nicht �berschreiben! Bei allen anderen Eintr�gen hier
            * muss buf erneut gef�llt werden, da die betreffende Information in nur
            * einer Zeile steht, und nun die n�chste gelesen werden muss. */

        case P_NEXT:
            f = NULL;
            lang = default_locale;
            b = in->getbuf(in->data);
            break;

        default:
            b = in->getbuf(in->data);
            break;
        }
    }

    log_info("done reading orders for %d factions", nfactions);
    return 0;
}

static const char * file_getbuf(void *data)
{
    FILE *F = (FILE *)data;
    return getbuf(F);
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
        if (!checkpasswd(f, password)) {
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
    unit * u = state->u;
    order *ord;

    ord = parse_order(str, u->faction->locale);
    if (ord) {
        *state->next_order = ord;
        state->next_order = &ord->next;
    }
    else {
        ADDMSG(&u->faction->msgs, msg_message("parse_error", "unit command", u, str));
    }
}


int parseorders(FILE *F)
{
    char buf[2048];
    int done = 0, err = 0;
    OP_Parser parser;
    parser_state state = { NULL, NULL };

    parser = OP_ParserCreate();
    if (!parser) {
        /* TODO: error message */
        return errno;
    }
    OP_SetUnitHandler(parser, handle_unit);
    OP_SetFactionHandler(parser, handle_faction);
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
            err = -1;
            break;
        }
    }
    OP_ParserFree(parser);
    return err;
}

int readorders(FILE *F)
{
#undef NEW_PARSER
#ifdef NEW_PARSER
    return parseorders(F);
#else
    input in;
    int result;

    in.getbuf = file_getbuf;
    in.data = F;
    result = read_orders(&in);
    return result;
#endif
}
