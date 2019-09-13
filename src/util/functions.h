#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#ifdef __cplusplus
extern "C" {
#endif

    typedef void(*pf_generic) (void);

    pf_generic get_function(const char *name);
    void register_function(pf_generic fun, const char *name);
    void free_functions(void);

#ifdef __cplusplus
}
#endif
#endif
