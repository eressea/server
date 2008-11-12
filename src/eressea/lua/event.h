#ifndef LUA_EVENT_H
#define LUA_EVENT_H
class event {
public:
  event(char * m, struct event_arg * a) : args(a), msg(m) {}

  const char * get_message(int i) const { return msg; }
  const char * get_type(int i) const;
  struct unit * get_unit(int i) const;
  const char * get_string(int i) const;
  int get_int(int i) const;

private:
  struct event_arg * args;
  char * msg;
};

#endif
