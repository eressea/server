#include "unicode.h"

#include <utf8proc.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <wctype.h>
#include <ctype.h>

static bool unicode_trimmed(const utf8proc_property_t* property)
{
    return
        property->category == UTF8PROC_CATEGORY_CF ||
        property->category == UTF8PROC_CATEGORY_CC ||
        property->category == UTF8PROC_CATEGORY_ZS ||
        property->category == UTF8PROC_CATEGORY_ZL ||
        property->category == UTF8PROC_CATEGORY_ZP ||
        property->bidi_class == UTF8PROC_BIDI_CLASS_WS ||
        property->bidi_class == UTF8PROC_BIDI_CLASS_B ||
        property->bidi_class == UTF8PROC_BIDI_CLASS_S ||
        property->bidi_class == UTF8PROC_BIDI_CLASS_BN;
}

static void move_bytes(char* out, const char* begin, size_t bytes)
{
    if (out != begin && bytes > 0) {
        memmove(out, begin, bytes);
    }
}

void utf8_clean(char* buf)
{
    char* str = buf;
    char* out = buf;
    char* begin = str;
    const size_t len = strlen(buf);
    utf8proc_ssize_t ulen = (utf8proc_ssize_t)len;
    while (ulen > 0 && *str) {
        utf8proc_int32_t codepoint;
        utf8proc_ssize_t size = utf8proc_iterate((const utf8proc_uint8_t*)str, ulen, &codepoint);
        if (size <= 0) {
            break;
        }
        else if (size == 1) {
            if (iscntrl(*str)) {
                move_bytes(out, begin, str - begin);
                out += (str - begin);
                begin = str + size;
            }
        }
        else {
            const utf8proc_property_t* property = utf8proc_get_property(codepoint);
            if (property->bidi_class == UTF8PROC_BIDI_CLASS_S || property->bidi_class == UTF8PROC_BIDI_CLASS_B) {
                move_bytes(out, begin, str - begin);
                out += (str - begin);
                begin = str + size;
            }
        }
        str += size;
    }
    move_bytes(out, begin, str - begin);
    out[str - begin] = 0;
}

int
utf8_decode(wchar_t * ucs4_character, const char * utf8_string,
    size_t * length)
{
    const char * str = utf8_string;
    const size_t len = strlen(str);
    utf8proc_ssize_t ulen = (utf8proc_ssize_t)len;
    utf8proc_int32_t codepoint;
    utf8proc_ssize_t size = utf8proc_iterate((const utf8proc_uint8_t*)str, ulen, &codepoint);
    if (size <= 0) {
        return EILSEQ;
    }
    *length = (size_t)size;
    *ucs4_character = (wchar_t)codepoint;
    return 0;
}

size_t utf8_trim(char* buf)
{
    char* str = buf;
    char* out = buf;
    const size_t len = strlen(buf);
    utf8proc_ssize_t ulen = (utf8proc_ssize_t)len;
    // @var begin: first byte in current printable sequence
    // @var end: one-after last byte in current sequence
    // @var rtrim: potential first byte of right-trim space
    char* begin = NULL, * end = NULL, * ltrim = str, *rtrim = NULL;
    while (ulen > 0 && *str) {
        utf8proc_int32_t codepoint;
        utf8proc_ssize_t size = utf8proc_iterate((const utf8proc_uint8_t*)str, ulen, &codepoint);
        if (size > 0) {
            const utf8proc_property_t* property = utf8proc_get_property(codepoint);
            if (str == ltrim) {
                // we are still trimming on the left
                if (unicode_trimmed(property)) {
                    ltrim += size;
                }
                else {
                    // we are finished trimming, start the first sequence here:
                    ltrim = NULL;
                    begin = str;
                    end = str + size;
                }
            }
            else {
                // we are still trying to extend the sequence:
                if (property->bidi_class != UTF8PROC_BIDI_CLASS_WS) {
                    // include in sequence:
                    end = str + size;
                    if (unicode_trimmed(property)) {
                        // could be trailing junk, set rtrim lower bound
                        if (!rtrim) {
                            rtrim = str;
                        }
                    }
                    else {
                        rtrim = NULL;
                    }
                }
                else {
                    // whitespace, might start right-trim or end of sequence
                    if (!rtrim) {
                        rtrim = str;
                    }
                }
            }
        }
        else {
            *str = 0;
            return EILSEQ;
        }
        str += size;
    }
    if (begin && end) {
        ptrdiff_t plen;
        if (rtrim && rtrim < end) {
            end = rtrim;
        }
        plen = end - begin;
        move_bytes(out, begin, plen);
        out[plen] = 0;
        return len - plen;
    }
    return 0;
}

int
utf8_from_latin1(char * dst, size_t * outlen, const char *in,
    size_t * inlen)
{
    int is = (int)*inlen;
    int os = (int)*outlen;
    const char *ip = in;
    unsigned char *out = (unsigned char *)dst, *op = out;

    while (ip - in < is) {
        unsigned char c = *ip;
        if (c > 0xBF) {
            if (op - out >= os - 1)
                break;
            *op++ = 0xC3;
            *op++ = (unsigned char)(c - 64);
        }
        else if (c > 0x7F) {
            if (op - out >= os - 1)
                break;
            *op++ = 0xC2;
            *op++ = c;
        }
        else {
            if (op - out >= os)
                break;
            *op++ = c;
        }
        ++ip;
    }
    *outlen = op - out;
    *inlen = ip - in;
    return (int)*outlen;
}

int utf8_strcasecmp(const char * a, const char *b)
{
    if (strcmp(a, b) == 0) {
        return 0;
    }
    utf8proc_ssize_t len_a = (utf8proc_ssize_t)strlen(a);
    utf8proc_ssize_t len_b = (utf8proc_ssize_t)strlen(b);
    utf8proc_uint8_t * ap = (utf8proc_uint8_t *)a;
    utf8proc_uint8_t * bp = (utf8proc_uint8_t *)b;
    while (*ap && *bp) {
        utf8proc_int32_t ca, cb;
        utf8proc_ssize_t size_a, size_b;
        size_a = utf8proc_iterate(ap, len_a, &ca);
        size_b = utf8proc_iterate(bp, len_b, &cb);
        if (size_a < 0) {
            return -1;
        }
        if (size_b < 0) {
            return 1;
        }
        ca = utf8proc_tolower(ca);
        cb = utf8proc_tolower(cb);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
        ap += size_a;
        bp += size_b;
        len_a -= size_a;
        len_b -= size_b;
    }
    if (*ap) return 1;
    if (*bp) return -1;
    return 0;
}


/** Convert a UTF-8 encoded character to CP437. */
int
utf8_to_cp437(unsigned char *cp_character, const char * utf8_string,
    size_t * length)
{
    wchar_t ucs4_character;
    int result;

    result = utf8_decode(&ucs4_character, utf8_string, length);
    if (result != 0) {
        /* pass decoding characters upstream */
        return result;
    }

    if (ucs4_character < 0x7F) {
        *cp_character = (unsigned char)ucs4_character;
    }
    else {
        struct {
            wchar_t ucs4;
            unsigned char cp437;
        } xref[160] = {
            { 0x00A0, 255 },
            { 0x00A1, 173 },
            { 0x00A2, 155 },
            { 0x00A3, 156 },
            { 0x00A5, 157 },
            { 0x00A7, 21 },
            { 0x00AA, 166 },
            { 0x00AB, 174 },
            { 0x00AC, 170 },
            { 0x00B0, 248 },
            { 0x00B1, 241 },
            { 0x00B2, 253 },
            { 0x00B5, 230 },
            { 0x00B6, 20 },
            { 0x00B7, 250 },
            { 0x00BA, 167 },
            { 0x00BB, 175 },
            { 0x00BC, 172 },
            { 0x00BD, 171 },
            { 0x00BF, 168 },
            { 0x00C4, 142 },
            { 0x00C5, 143 },
            { 0x00C6, 146 },
            { 0x00C7, 128 },
            { 0x00C9, 144 },
            { 0x00D1, 165 },
            { 0x00D6, 153 },
            { 0x00DC, 154 },
            { 0x00DF, 225 },
            { 0x00E0, 133 },
            { 0x00E1, 160 },
            { 0x00E2, 131 },
            { 0x00E4, 132 },
            { 0x00E5, 134 },
            { 0x00E6, 145 },
            { 0x00E7, 135 },
            { 0x00E8, 138 },
            { 0x00E9, 130 },
            { 0x00EA, 136 },
            { 0x00EB, 137 },
            { 0x00EC, 141 },
            { 0x00ED, 161 },
            { 0x00EE, 140 },
            { 0x00EF, 139 },
            { 0x00F1, 164 },
            { 0x00F2, 149 },
            { 0x00F3, 162 },
            { 0x00F4, 147 },
            { 0x00F6, 148 },
            { 0x00F7, 246 },
            { 0x00F9, 151 },
            { 0x00FA, 163 },
            { 0x00FB, 150 },
            { 0x00FC, 129 },
            { 0x00FF, 152 },
            { 0x0192, 159 },
            { 0x0393, 226 },
            { 0x0398, 233 },
            { 0x03A3, 228 },
            { 0x03A6, 232 },
            { 0x03A9, 234 },
            { 0x03B1, 224 },
            { 0x03B4, 235 },
            { 0x03B5, 238 },
            { 0x03C0, 227 },
            { 0x03C3, 229 },
            { 0x03C4, 231 },
            { 0x03C6, 237 },
            { 0x2022, 7 },
            { 0x203C, 19 },
            { 0x207F, 252 },
            { 0x20A7, 158 },
            { 0x2190, 27 },
            { 0x2191, 24 },
            { 0x2192, 26 },
            { 0x2193, 25 },
            { 0x2194, 29 },
            { 0x2195, 18 },
            { 0x21A8, 23 },
            { 0x2219, 249 },
            { 0x221A, 251 },
            { 0x221E, 236 },
            { 0x221F, 28 },
            { 0x2229, 239 },
            { 0x2248, 247 },
            { 0x2261, 240 },
            { 0x2264, 243 },
            { 0x2265, 242 },
            { 0x2302, 127 },
            { 0x2310, 169 },
            { 0x2320, 244 },
            { 0x2321, 245 },
            { 0x2500, 196 },
            { 0x2502, 179 },
            { 0x250C, 218 },
            { 0x2510, 191 },
            { 0x2514, 192 },
            { 0x2518, 217 },
            { 0x251C, 195 },
            { 0x2524, 180 },
            { 0x252C, 194 },
            { 0x2534, 193 },
            { 0x253C, 197 },
            { 0x2550, 205 },
            { 0x2551, 186 },
            { 0x2552, 213 },
            { 0x2553, 214 },
            { 0x2554, 201 },
            { 0x2555, 184 },
            { 0x2556, 183 },
            { 0x2557, 187 },
            { 0x2558, 212 },
            { 0x2559, 211 },
            { 0x255A, 200 },
            { 0x255B, 190 },
            { 0x255C, 189 },
            { 0x255D, 188 },
            { 0x255E, 198 },
            { 0x255F, 199 },
            { 0x2560, 204 },
            { 0x2561, 181 },
            { 0x2562, 182 },
            { 0x2563, 185 },
            { 0x2564, 209 },
            { 0x2565, 210 },
            { 0x2566, 203 },
            { 0x2567, 207 },
            { 0x2568, 208 },
            { 0x2569, 202 },
            { 0x256A, 216 },
            { 0x256B, 215 },
            { 0x256C, 206 },
            { 0x2580, 223 },
            { 0x2584, 220 },
            { 0x2588, 219 },
            { 0x258C, 221 },
            { 0x2590, 222 },
            { 0x2591, 176 },
            { 0x2592, 177 },
            { 0x2593, 178 },
            { 0x25A0, 254 },
            { 0x25AC, 22 },
            { 0x25B2, 30 },
            { 0x25BA, 16 },
            { 0x25BC, 31 },
            { 0x25C4, 17 },
            { 0x25CB, 9 },
            { 0x25D8, 8 },
            { 0x25D9, 10 },
            { 0x263A, 1 },
            { 0x263B, 2 },
            { 0x263C, 15 },
            { 0x2640, 12 },
            { 0x2642, 11 },
            { 0x2660, 6 },
            { 0x2663, 5 },
            { 0x2665, 3 },
            { 0x2666, 4 },
            { 0x266A, 13 },
            { 0x266B, 14 }
        };
        int l = 0, r = 160;
        while (l != r) {
            int m = (l + r) / 2;
            if (xref[m].ucs4 == ucs4_character) {
                *cp_character = (char)xref[m].cp437;
                break;
            }
            else if (xref[m].ucs4 < ucs4_character) {
                if (l == m) l = r;
                else l = m;
            }
            else {
                if (r == m) r = l;
                else r = m;
            }
        }
        if (l == r) {
            *cp_character = '?';
        }
    }
    return 0;
}

/** Convert a UTF-8 encoded character to ASCII, with '?' replacements. */
int utf8_to_ascii(unsigned char *cp_character, const char * utf8_string,
    size_t *length)
{
    wchar_t ucs4_character;
    int result = utf8_decode(&ucs4_character, utf8_string, length);
    if (result == 0) {
        if (*length > 1) {
            *cp_character = '?';
        }
        else {
            *cp_character = (unsigned char)ucs4_character;
        }
    }
    return result;
}

/** Convert a UTF-8 encoded character to CP1252. */
int utf8_to_cp1252(unsigned char *cp_character, const char * utf8_string,
    size_t * length)
{
    wchar_t ucs4_character;
    int result;

    result = utf8_decode(&ucs4_character, utf8_string, length);
    if (result != 0) {
        /* pass decoding characters upstream */
        return result;
    }

    if (ucs4_character <= 0x7F || ucs4_character >= 0xA0) {
        *cp_character = (char)ucs4_character;
    }
    else {
        struct {
            wchar_t ucs4;
            unsigned char cp;
        } xref[] = {
            { 0x0081, 0x81 },
            { 0x008d, 0x8d },
            { 0x008f, 0x8f },
            { 0x0090, 0x90 },
            { 0x009d, 0x9d },
            { 0x0152, 0x8c },
            { 0x0153, 0x9c },
            { 0x0160, 0x8a },
            { 0x0161, 0x9a },
            { 0x0178, 0x9f },
            { 0x017d, 0x8e },
            { 0x017e, 0x9e },
            { 0x0192, 0x83 },
            { 0x02c6, 0x88 },
            { 0x02dc, 0x98 },
            { 0x2013, 0x96 },
            { 0x2014, 0x97 },
            { 0x2018, 0x91 },
            { 0x2019, 0x92 },
            { 0x201a, 0x82 },
            { 0x201c, 0x93 },
            { 0x201d, 0x94 },
            { 0x201e, 0x84 },
            { 0x2022, 0x95 },
            { 0x2026, 0x85 },
            { 0x2020, 0x86 },
            { 0x2021, 0x87 },
            { 0x2030, 0x89 },
            { 0x203a, 0x9b },
            { 0x2039, 0x8b },
            { 0x20ac, 0x80 },
            { 0x2122, 0x99 }
        };
        int l = 0, r = sizeof(xref) / sizeof(xref[0]);
        while (l != r) {
            int m = (l + r) / 2;
            if (xref[m].ucs4 == ucs4_character) {
                *cp_character = (char)xref[m].cp;
                break;
            }
            else if (xref[m].ucs4 < ucs4_character)
                l = m;
            else
                r = m;
        }
        if (l == r) {
            *cp_character = '?';
        }
    }
    return 0;
}

const char* utf8_ltrim(const char* str)
{
    if (str == NULL) {
        return NULL;
    }
    else {
        const size_t len = strlen(str);
        utf8proc_ssize_t ulen = (utf8proc_ssize_t)len;
        while (ulen > 0 && *str) {
            utf8proc_int32_t codepoint;
            utf8proc_ssize_t size = utf8proc_iterate((const utf8proc_uint8_t*)str, ulen, &codepoint);

            if (size < 1) {
                return NULL;
            }
            else {
                const utf8proc_property_t* property = utf8proc_get_property(codepoint);
                if (!unicode_trimmed(property)) {
                    break;
                }
                str += size;
            }
        }
        return str;

    }
}
