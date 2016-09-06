#include <platform.h>
#include <kernel/config.h>
#include "direction.h"
#include "vortex.h"

#include "util/language.h"
#include "util/umlaut.h"

#include <string.h>

void init_direction(const struct locale *lang, direction_t dir, const char *str) {
    void **tokens = get_translations(lang, UT_DIRECTIONS);
    variant token;
    token.i = dir;
    addtoken((struct tnode **)tokens, str, token);
}

void init_directions(struct locale *lang) {
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

    register_special_direction(lang, "vortex");

    for (i = 0; dirs[i].direction != NODIRECTION; ++i) {
        const char *str = locale_string(lang, dirs[i].name, false);
        if (str) {
            variant token;
            token.i = dirs[i].direction;
            addtoken((struct tnode **)tokens, str, token);
        }
    }
}

direction_t get_direction(const char *s, const struct locale *lang)
{
    void **tokens = get_translations(lang, UT_DIRECTIONS);
    variant token;

    if (findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
        return (direction_t)token.i;
    }
    return NODIRECTION;
}

direction_t finddirection(const char *str) {
    int i;
    for (i = 0; i != MAXDIRECTIONS + 2; ++i) {
        if (directions[i] && strcmp(str, directions[i]) == 0) {
            return (direction_t)i;
        }
    }
    return NODIRECTION;
}

const char * directions[MAXDIRECTIONS + 2] = {
    "northwest", "northeast", "east", "southeast", "southwest", "west", 0, "pause"
};

