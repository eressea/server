#pragma once

#ifndef PREFIX_H
#define PREFIX_H

#ifdef __cplusplus
extern "C" {
#endif
    extern char **race_prefixes;
    /* zero-terminated array of valid prefixes */

    int add_raceprefix(const char *);
    void free_prefixes(void);

#ifdef __cplusplus
}
#endif
#endif
