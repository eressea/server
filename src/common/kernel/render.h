/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <kernel/message.h>
#include <util/nrmessage.h>
#include <util/message.h>

struct message;

/* TODO: this could be nicer and faster 
 * call with MSG(("msg_name", "param", p), buf, faction). */
#define MSG(makemsg, buf, size, loc, ud) { struct message * m = msg_message makemsg; nr_render(m, loc, buf, size, ud); msg_release(m); }
#define RENDER(f, buf, size, mcreate) { struct message * m = msg_message mcreate; nr_render(m, f->locale, buf, size, f); msg_release(m); }
