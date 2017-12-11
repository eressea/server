#ifndef H_DIRECTION_H
#define H_DIRECTION_H

#ifdef __cplusplus
extern "C"
{
#endif

    struct locale;

    typedef enum {
        D_NORTHWEST,
        D_NORTHEAST,
        D_EAST,
        D_SOUTHEAST,
        D_SOUTHWEST,
        D_WEST,
        MAXDIRECTIONS,
        D_PAUSE,
        D_SPECIAL,
        NODIRECTION = -1
    } direction_t;

    extern const char *shortdirections[MAXDIRECTIONS];

    direction_t get_direction(const char *s, const struct locale *);
    void init_directions(struct locale *lang);
    void init_direction(const struct locale *lang, direction_t dir, const char *str);

    direction_t finddirection(const char *str);

    extern const char * directions[];

#ifdef __cplusplus
}
#endif
#endif
