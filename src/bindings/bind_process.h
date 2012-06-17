#ifndef BIND_ERESSEA_PROCESS_H
#define BIND_ERESSEA_PROCESS_H
#ifdef __cplusplus
extern "C" {
#endif

void process_produce(void);
void process_markets(void);
void process_update_long_order(void);

void process_make_temp(void);
void process_settings(void);
void process_ally(void);
void process_prefix(void);
void process_setstealth(void);
void process_status(void);
void process_name(void);
void process_group(void);
void process_origin(void);
void process_quit(void);
void process_study(void);
void process_movement(void);
void process_use(void);
void process_battle(void);
void process_siege(void);
void process_leave(void);
void process_maintenance(void);
void process_promote(void);
void process_renumber(void);
void process_restack(void);
void process_setspells(void);
void process_sethelp(void);

#ifdef __cplusplus
}
#endif
#endif
