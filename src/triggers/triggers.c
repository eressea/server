/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>

/* triggers includes */
#include <triggers/changefaction.h>
#include <triggers/changerace.h>
#include <triggers/createcurse.h>
#include <triggers/createunit.h>
#include <triggers/gate.h>
#include <triggers/unguard.h>
#include <triggers/giveitem.h>
#include <triggers/killunit.h>
#include <triggers/shock.h>
#include <triggers/timeout.h>
#include <triggers/unitmessage.h>
#include <triggers/clonedied.h>

/* util includes */
#include <util/event.h>

/* libc includes */
#include <stdio.h>

void register_triggers(void)
{
    tt_register(&tt_changefaction);
    tt_register(&tt_changerace);
    tt_register(&tt_createcurse);
    tt_register(&tt_createunit);
    tt_register(&tt_gate);
    tt_register(&tt_unguard);
    tt_register(&tt_giveitem);
    tt_register(&tt_killunit);
    tt_register(&tt_shock);
    tt_register(&tt_unitmessage);
    tt_register(&tt_timeout);
    tt_register(&tt_clonedied);
}
