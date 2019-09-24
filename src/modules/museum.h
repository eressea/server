#ifndef HEADER_MUSEUM_H
#define HEADER_MUSEUM_H

#ifdef __cplusplus
extern "C" {
#endif

    extern struct attrib_type at_warden;
    extern struct attrib_type at_museumexit;
    extern struct attrib_type at_museumgivebackcookie;
    extern struct attrib_type at_museumgiveback;

    typedef struct {
        int warden_no;
        int cookie;
    } museumgivebackcookie;

    typedef struct {
        int cookie;
        struct item *items;
    } museumgiveback;

    extern void register_museum(void);

#ifdef __cplusplus
}
#endif
#endif
