#include <kernel/messages.h>
#include <util/nrmessage.h>
#include <util/message.h>

struct message;

/* TODO: this could be nicer and faster 
 * call with MSG(("msg_name", "param", p), buf, faction). */
#define MSG(makemsg, buf, size, loc, ud) { struct message * mm = msg_message makemsg; nr_render(mm, loc, buf, size, ud); msg_release(mm); }
#define RENDER(f, buf, size, mcreate) { struct message * mr = msg_message mcreate; nr_render(mr, f->locale, buf, size, f); msg_release(mr); }
