#ifndef GOODIES_H
#define GOODIES_H

#ifdef __cplusplus
extern "C" {
#endif

    int check_email(const char *newmail);

    int *intlist_init(void);
    int *intlist_add(int *i_p, int i);
    int *intlist_find(int *i_p, int i);

#ifdef __cplusplus
}
#endif
#endif                          /* GOODIES_H */
