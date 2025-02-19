#define USE_DL_PREFIX
#define DL_MALLOC_IMPLEMENTATION
#include <dlmalloc.h>
#define STBDS_REALLOC(c,p,s) dlrealloc(p,s)
#define STBDS_FREE(c,p)      dlfree(p)
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#define GB_ALLOC(s) dlmalloc(s)
#define GB_FREE(p) dlfree(p)
#define GB_STRING_IMPLEMENTATION
#include <gb_string.h>

