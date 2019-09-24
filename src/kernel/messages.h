#ifndef H_KRNL_MESSAGE
#define H_KRNL_MESSAGE
#ifdef __cplusplus
extern "C" {
#endif

#include <kernel/types.h>
#include <util/message.h>

    struct faction;

    typedef struct mlist {
        struct mlist *next;
        struct message *msg;
    } mlist;

    typedef struct message_list {
        struct mlist *begin, **end;
    } message_list;

    void free_messagelist(struct mlist *msgs);

#define MESSAGE_MISSING_IGNORE  0
#define MESSAGE_MISSING_ERROR   1
#define MESSAGE_MISSING_REPLACE 2

    void message_handle_missing(int mode);

    struct message *msg_message(const char *name, const char *sig, ...);
    struct message *msg_feedback(const struct unit *, struct order *cmd,
        const char *name, const char *sig, ...);
    struct message *add_message(struct message_list **pm,
    struct message *m);
    void addmessage(struct region *r, struct faction *f, const char *s,
        msg_t mtype, int level);

    struct mlist ** merge_messages(message_list *ml, message_list *append);
    void split_messages(message_list *ml, struct mlist **split);

#define ADDMSG(msgs, mcreate) { message * mx = mcreate; if (mx) { add_message(msgs, mx); msg_release(mx); } }

    void syntax_error(const struct unit *u, struct order *ord);
    void cmistake(const struct unit *u, struct order *ord, int mno, int mtype);
    struct message * msg_error(const struct unit * u, struct order *ord, int mno);
#ifdef __cplusplus
}
#endif
#endif
