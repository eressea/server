#pragma once

#ifndef PREFIX_H
#define PREFIX_H

#ifdef __cplusplus
extern "C" {
#endif

    int add_raceprefix(const char *);
    char **race_prefixes; // zero-terminated array of valid prefixes
    void free_prefixes(void);

#ifdef __cplusplus
}
#endif
#endif
