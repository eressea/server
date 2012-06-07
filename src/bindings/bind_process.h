#ifndef BIND_ERESSEA_PROCESS_H
#define BIND_ERESSEA_PROCESS_H
#ifdef __cplusplus
extern "C" {
#endif

void process_produce(void);
void process_markets(void);
void process_make_temp(void);
void process_settings(void);
void process_group(void);
void process_origin(void);
void process_quit(void);

#ifdef __cplusplus
}
#endif
#endif
