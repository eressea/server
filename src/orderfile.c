#include <platform.h>
#include <kernel/config.h>
#include "orderfile.h"

#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <kernel/messages.h>

#include <util/base36.h>
#include <util/message.h>
#include <util/language.h>
#include <util/log.h>
#include <util/filereader.h>
#include <util/parser.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static unit *unitorders(input *in, faction *f)
{
    int i;
    unit *u;

    if (!f)
        return NULL;

    i = getid();
    u = findunitg(i, NULL);

    if (u && u->faction == f) {
        order **ordp;

        if (!fval(u, UFL_ORDERS)) {
            /* alle wiederholbaren, langen befehle werden gesichert: */
            fset(u, UFL_ORDERS);
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
        u->orders = 0;

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
    faction *f = NULL;
    int fid = getid();

    f = findfaction(fid);

    if (f != NULL && !fval(f, FFL_NPC)) {
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

    /* TODO: recognize UTF8 BOM */
    b = in->getbuf(in->data);

    /* Auffinden der ersten Partei, und danach abarbeiten bis zur letzten
    * Partei */

    while (b) {
        char token[128];
        const struct locale *lang = f ? f->locale : default_locale;
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
    return getbuf(F, ENCODING_UTF8);
}

int readorders(const char *filename)
{
    input in;
    int result;

    FILE *F = NULL;
    F = fopen(filename, "r");
    if (!F) {
        perror(filename);
        return -1;
    }
    log_info("reading orders from %s", filename);
    in.getbuf = file_getbuf;
    in.data = F;
    result = read_orders(&in);
    fclose(F);
    return result;
}
