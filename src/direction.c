#include <platform.h>
#include <kernel/config.h>
#include "direction.h"

#include "util/language.h"
#include "util/umlaut.h"

void init_direction(const struct locale *lang, direction_t dir, const char *str) {
    void **tokens = get_translations(lang, UT_DIRECTIONS);
    variant token;
    token.i = dir;
    addtoken(tokens, str, token);
}

void init_directions(const struct locale *lang) {
    /* mit dieser routine kann man mehrere namen für eine direction geben,
     * das ist für die hexes ideal. */
    const struct {
        const char *name;
        direction_t direction;
    } dirs[] = {
        { "dir_ne", D_NORTHEAST },
        { "dir_nw", D_NORTHWEST }, 
        { "dir_se", D_SOUTHEAST }, 
        { "dir_sw", D_SOUTHWEST }, 
        { "dir_east", D_EAST }, 
        { "dir_west", D_WEST }, 
        { "northeast", D_NORTHEAST },
        { "northwest", D_NORTHWEST },
        { "southeast", D_SOUTHEAST },
        { "southwest", D_SOUTHWEST },
        { "east", D_EAST },
        { "west", D_WEST }, 
        { "PAUSE", D_PAUSE },
        { NULL, NODIRECTION }
    };
    int i;
    void **tokens = get_translations(lang, UT_DIRECTIONS);

    for (i = 0; dirs[i].direction != NODIRECTION; ++i) {
        variant token;
        const char *str = locale_string(lang, dirs[i].name);
        token.i = dirs[i].direction;
        addtoken(tokens, str, token);
    }
}

direction_t finddirection(const char *s, const struct locale *lang)
{
    void **tokens = get_translations(lang, UT_DIRECTIONS);
    variant token;

    if (findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
        return (direction_t)token.i;
    }
    return NODIRECTION;
}

