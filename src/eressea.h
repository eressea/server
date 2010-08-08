#ifndef H_ERESSEA_LIB
#define H_ERESSEA_LIB
#ifdef __cplusplus
extern "C" {
#endif

int eressea_init(void);
void eressea_done(void);
int eressea_run(const char * luafile, const char * entry_point);

#ifdef __cplusplus
}
#endif

#endif
