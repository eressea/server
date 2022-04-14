#pragma once
#ifndef H_KRNL_MESSAGE
#define H_KRNL_MESSAGE

#include <util/message.h>

#define MESSAGE_MISSING_IGNORE  0
#define MESSAGE_MISSING_ERROR   1
#define MESSAGE_MISSING_REPLACE 2

typedef struct mlist {
    struct mlist *next;
    struct message *msg;
} mlist;

typedef struct message_list {
    struct mlist *begin, **end;
} message_list;

typedef enum msg_t {                  /* Fehler und Meldungen im Report */
    MSG_BATTLE,
    MSG_EVENT,
    MSG_MOVE,
    MSG_INCOME,
    MSG_COMMERCE,
    MSG_PRODUCE,
    MSG_MESSAGE,
    MSG_MAGIC,
    MAX_MSG
} msg_t;

enum {                          /* Message-Level */
    ML_IMPORTANT,                 /* Sachen, die IMO erscheinen _muessen_ */
    ML_DEBUG,
    ML_MISTAKE,
    ML_WARN,
    ML_INFO,
    ML_MAX
};

struct faction;
struct unit;
struct region;
struct order;
struct message;
enum msg_t;

void message_handle_missing(int mode);
void free_messagelist(struct mlist *msgs);

struct message *msg_message(const char *name, const char *sig, ...);
struct message *msg_feedback(const struct unit *, struct order *cmd,
    const char *name, const char *sig, ...);
struct message *add_message(struct message_list **pm,
struct message *m);
void addmessage(struct region *r, struct faction *f, const char *s,
    enum msg_t mtype, int level);

struct mlist ** merge_messages(message_list *ml, message_list *append);
void split_messages(message_list *ml, struct mlist **split);

#define ADDMSG(msgs, mcreate) { struct message * mx = mcreate; if (mx) { add_message(msgs, mx); msg_release(mx); } }

void syntax_error(const struct unit *u, struct order *ord);
void cmistake(const struct unit *u, struct order *ord, int mno, int mtype);
struct message * msg_error(const struct unit * u, struct order *ord, int mno);
#endif
