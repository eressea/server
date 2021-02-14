#include "base36.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define USE_STRTOL
#ifdef USE_STRTOL
int atoi36(const char *str)
{
    assert(str);
    return (int)strtol(str, NULL, 36);
}
#else
int atoi36(const char *str)
{
    const unsigned char *s = (const unsigned char *)str;
    int i = 0, sign = 1;
    assert(s);
    if (!(*s))
        return 0;
    if (*s == '-') {
        sign = -1;
        ++s;
    }
    while (isalnum(*(unsigned char *)s)) {
        if (islower(*s))
            i = i * 36 + (*s) - 'a' + 10;
        else if (isdigit(*s))
            i = i * 36 + (*s) - '0';
        else if (isupper(*(unsigned char *)s))
            i = i * 36 + (*s) - 'A' + 10;
        else
            break;
        ++s;
    }
    if (i < 0)
        return 0;
    return i * sign;
}
#endif

const char *itoab_r(int i, int base, char *s, size_t len)
{
    char *dst;

    assert(len > 2);
    dst = s + len - 2;
    dst[1] = 0;
    if (i != 0) {
        int neg = 0;

        if (i < 0) {
            i = -i;
            neg = 1;
        }
        while (i && dst >= s) {
            int x = i % base;
            i = i / base;
            if (x < 10) {
                *(dst--) = (char)('0' + x);
            }
            else if ('a' + x - 10 == 'l') {
                *(dst--) = 'L';
            }
            else {
                *(dst--) = (char)('a' + (x - 10));
            }
        }
        if (dst > s) {
            if (neg) {
                *(dst) = '-';
            }
            else {
                ++dst;
            }
        }
        else {
            fprintf(stderr, "static buffer exhausted, itoab(%d, %d)", i, base);
            assert(i == 0 || !"itoab: static buffer exhausted");
        }
    }
    else {
        dst[0] = '0';
    }

    return dst;
}

const char *itoa36_r(int i, char *result, size_t len)
{
    return itoab_r(i, 36, result, len);
}
 
const char *itoab(int i, int base)
{
    static char sstr[80];
    char *s;
    static int index = 0;         /* STATIC_XCALL: used across calls */

    s = sstr + (index * 20);
    index = (index + 1) & 3;      /* quick for % 4 */

    return itoab_r(i, base, s, 20);
}

const char *itoa36(int i)
{
    return itoab(i, 36);
}

const char *itoa10(int i)
{
    return itoab(i, 10);
}
