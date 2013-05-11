#ifndef BIND_ERESSEA_H
#define BIND_ERESSEA_H
#ifdef __cplusplus
extern "C" {
#endif

void eressea_free_game(void);
int eressea_read_game(const char * filename);
int eressea_write_game(const char * filename);
int eressea_read_orders(const char * filename);

#ifdef __cplusplus
}
#endif
#endif
