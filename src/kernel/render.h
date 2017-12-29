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

#include <kernel/messages.h>
#include <util/nrmessage.h>
#include <util/message.h>

struct message;

/* TODO: this could be nicer and faster 
 * call with MSG(("msg_name", "param", p), buf, faction). */
#define MSG(makemsg, buf, size, loc, ud) { struct message * mm = msg_message makemsg; nr_render(mm, loc, buf, size, ud); msg_release(mm); }
#define RENDER(f, buf, size, mcreate) { struct message * mr = msg_message mcreate; nr_render(mr, f->locale, buf, size, f); msg_release(mr); }
