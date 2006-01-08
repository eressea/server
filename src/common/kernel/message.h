/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_KRNL_MESSAGE
#define H_KRNL_MESSAGE
#ifdef __cplusplus
extern "C" {
#endif

#include <util/message.h>

struct message;
struct messageclass;
struct faction;
struct msglevel;

struct message_type;

typedef struct message_list {
  struct mlist {
    struct mlist * next;
    struct message *msg;
  } * begin, **end;
} message_list;

extern void free_messagelist(message_list * msgs);

typedef struct messageclass {
  struct messageclass * next;
  const char * name;
} messageclass;

typedef struct msglevel {
	/* used to set specialized msg-levels */
	struct msglevel *next;
	const struct message_type *type;
	int level;
} msglevel;

extern struct message * msg_message(const char * name, const char* sig, ...);
extern struct message * msg_feedback(const struct unit *, struct order *cmd,
                                  const char * name, const char* sig, ...);
extern struct message * add_message(struct message_list** pm, struct message * m);

/* message sections */
extern struct messageclass * msgclasses;
extern const struct messageclass * mc_add(const char * name);
extern const struct messageclass * mc_find(const char * name);

#define ADDMSG(msgs, mcreate) { message * m = mcreate; if (m) { assert(m->refcount>=1); add_message(msgs, m); msg_release(m); } }

extern void cmistake(const struct unit * u, struct order *ord, int mno, int mtype);
extern void mistake(const struct unit * u, struct order *ord, const char *text, int mtype);

#ifdef __cplusplus
}
#endif
#endif
