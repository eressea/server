#ifndef RESOLVE_H
#define RESOLVE_H

struct storage;
struct gamedata;

#ifdef __cplusplus
extern "C" {
#endif

    typedef void *(*resolve_fun) (int id, void *data);
    typedef int(*read_fun) (struct gamedata * data);

    void ur_add(int id, void **addr, resolve_fun fun);
    void resolve(int id, void *data);

#ifdef __cplusplus
}
#endif
#endif
