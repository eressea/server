/*
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2007   |
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *
 */

#include <platform.h>
#include "unicode.h"

#include <errno.h>
#include <string.h>
#include <wctype.h>

#define B00000000 0x00
#define B10000000 0x80
#define B11000000 0xC0
#define B11100000 0xE0
#define B11110000 0xF0
#define B11111000 0xF8
#define B11111100 0xFC
#define B11111110 0xFE

#define B00111111 0x3F
#define B00011111 0x1F
#define B00001111 0x0F
#define B00000111 0x07
#define B00000011 0x03
#define B00000001 0x01

int unicode_utf8_tolower(utf8_t * op, size_t outlen, const utf8_t * ip)
{
    while (*ip) {
        ucs4_t ucs = *ip;
        ucs4_t low;
        size_t size = 1;

        if (ucs & 0x80) {
            int ret = unicode_utf8_to_ucs4(&ucs, ip, &size);
            if (ret != 0) {
                return ret;
            }
        }
        if (size > outlen) {
            return ENOMEM;
        }
        low = towlower((wint_t)ucs);
        if (low == ucs) {
            memcpy(op, ip, size);
            ip += size;
            op += size;
            outlen -= size;
        }
        else {
            ip += size;
            unicode_ucs4_to_utf8(op, &size, low);
            op += size;
            outlen -= size;
        }
    }

    if (outlen <= 0) {
        return ENOMEM;
    }
    *op = 0;
    return 0;
}

int
unicode_latin1_to_utf8(utf8_t * dst, size_t * outlen, const char *in,
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

int unicode_utf8_strcasecmp(const utf8_t * a, const utf8_t *b)
{
    while (*a && *b) {
        int ret;
        size_t size;
        ucs4_t ucsa = *a, ucsb = *b;

        if (ucsa & 0x80) {
            ret = unicode_utf8_to_ucs4(&ucsa, a, &size);
            if (ret != 0)
                return -1;
            a += size;
        }
        else
            ++a;
        if (ucsb & 0x80) {
            ret = unicode_utf8_to_ucs4(&ucsb, b, &size);
            if (ret != 0)
                return -1;
            b += size;
        }
        else
            ++b;

        if (ucsb != ucsa) {
            ucsb = towlower((wint_t)ucsb);
            ucsa = towlower((wint_t)ucsa);
            if (ucsb < ucsa)
                return 1;
            if (ucsb > ucsa)
                return -1;
        }
    }
    if (*b)
        return -1;
    if (*a)
        return 1;
    return 0;
}

/* Convert a UCS-4 character to UTF-8. */
int
unicode_ucs4_to_utf8(utf8_t * utf8_character, size_t * size,
    ucs4_t ucs4_character)
{
    int utf8_bytes;

    if (ucs4_character <= 0x0000007F) {
        /* 0xxxxxxx */
        utf8_bytes = 1;
        utf8_character[0] = (char)ucs4_character;
    }
    else if (ucs4_character <= 0x000007FF) {
        /* 110xxxxx 10xxxxxx */
        utf8_bytes = 2;
        utf8_character[0] = (char)((ucs4_character >> 6) | B11000000);
        utf8_character[1] = (char)((ucs4_character & B00111111) | B10000000);
    }
    else if (ucs4_character <= 0x0000FFFF) {
        /* 1110xxxx 10xxxxxx 10xxxxxx */
        utf8_bytes = 3;
        utf8_character[0] = (char)((ucs4_character >> 12) | B11100000);
        utf8_character[1] = (char)(((ucs4_character >> 6) & B00111111) | B10000000);
        utf8_character[2] = (char)((ucs4_character & B00111111) | B10000000);
    }
    else if (ucs4_character <= 0x001FFFFF) {
        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        utf8_bytes = 4;
        utf8_character[0] = (char)((ucs4_character >> 18) | B11110000);
        utf8_character[1] =
            (char)(((ucs4_character >> 12) & B00111111) | B10000000);
        utf8_character[2] = (char)(((ucs4_character >> 6) & B00111111) | B10000000);
        utf8_character[3] = (char)((ucs4_character & B00111111) | B10000000);
    }
    else if (ucs4_character <= 0x03FFFFFF) {
        /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        utf8_bytes = 5;
        utf8_character[0] = (char)((ucs4_character >> 24) | B11111000);
        utf8_character[1] =
            (char)(((ucs4_character >> 18) & B00111111) | B10000000);
        utf8_character[2] =
            (char)(((ucs4_character >> 12) & B00111111) | B10000000);
        utf8_character[3] = (char)(((ucs4_character >> 6) & B00111111) | B10000000);
        utf8_character[4] = (char)((ucs4_character & B00111111) | B10000000);
    }
    else if (ucs4_character <= 0x7FFFFFFF) {
        /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        utf8_bytes = 6;
        utf8_character[0] = (char)((ucs4_character >> 30) | B11111100);
        utf8_character[1] =
            (char)(((ucs4_character >> 24) & B00111111) | B10000000);
        utf8_character[2] =
            (char)(((ucs4_character >> 18) & B00111111) | B10000000);
        utf8_character[3] =
            (char)(((ucs4_character >> 12) & B00111111) | B10000000);
        utf8_character[4] = (char)(((ucs4_character >> 6) & B00111111) | B10000000);
        utf8_character[5] = (char)((ucs4_character & B00111111) | B10000000);
    }
    else {
        return EILSEQ;
    }

    *size = utf8_bytes;

    return 0;
}

/* Convert a UTF-8 encoded character to UCS-4. */
int
unicode_utf8_to_ucs4(ucs4_t * ucs4_character, const utf8_t * utf8_string,
    size_t * length)
{
    utf8_t utf8_character = utf8_string[0];

    /* Is the character in the ASCII range? If so, just copy it to the
       output. */
    if (~utf8_character & 0x80) {
        *ucs4_character = utf8_character;
        *length = 1;
    }
    else if ((utf8_character & 0xE0) == 0xC0) {
        /* A two-byte UTF-8 sequence. Make sure the other byte is good. */
        if (utf8_string[1] != '\0' && (utf8_string[1] & 0xC0) != 0x80) {
            return EILSEQ;
        }

        *ucs4_character =
            ((utf8_string[1] & 0x3F) << 0) + ((utf8_character & 0x1F) << 6);
        *length = 2;
    }
    else if ((utf8_character & 0xF0) == 0xE0) {
        /* A three-byte UTF-8 sequence. Make sure the other bytes are
           good. */
        if ((utf8_string[1] != '\0') &&
            (utf8_string[1] & 0xC0) != 0x80 &&
            (utf8_string[2] != '\0') && (utf8_string[2] & 0xC0) != 0x80) {
            return EILSEQ;
        }

        *ucs4_character =
            ((utf8_string[2] & 0x3F) << 0) +
            ((utf8_string[1] & 0x3F) << 6) + ((utf8_character & 0x0F) << 12);
        *length = 3;
    }
    else if ((utf8_character & 0xF8) == 0xF0) {
        /* A four-byte UTF-8 sequence. Make sure the other bytes are
           good. */
        if ((utf8_string[1] != '\0') &&
            (utf8_string[1] & 0xC0) != 0x80 &&
            (utf8_string[2] != '\0') &&
            (utf8_string[2] & 0xC0) != 0x80 &&
            (utf8_string[3] != '\0') && (utf8_string[3] & 0xC0) != 0x80) {
            return EILSEQ;
        }

        *ucs4_character =
            ((utf8_string[3] & 0x3F) << 0) +
            ((utf8_string[2] & 0x3F) << 6) +
            ((utf8_string[1] & 0x3F) << 12) + ((utf8_character & 0x07) << 18);
        *length = 4;
    }
    else if ((utf8_character & 0xFC) == 0xF8) {
        /* A five-byte UTF-8 sequence. Make sure the other bytes are
           good. */
        if ((utf8_string[1] != '\0') &&
            (utf8_string[1] & 0xC0) != 0x80 &&
            (utf8_string[2] != '\0') &&
            (utf8_string[2] & 0xC0) != 0x80 &&
            (utf8_string[3] != '\0') &&
            (utf8_string[3] & 0xC0) != 0x80 &&
            (utf8_string[4] != '\0') && (utf8_string[4] & 0xC0) != 0x80) {
            return EILSEQ;
        }

        *ucs4_character =
            ((utf8_string[4] & 0x3F) << 0) +
            ((utf8_string[3] & 0x3F) << 6) +
            ((utf8_string[2] & 0x3F) << 12) +
            ((utf8_string[1] & 0x3F) << 18) + ((utf8_character & 0x03) << 24);
        *length = 5;
    }
    else if ((utf8_character & 0xFE) == 0xFC) {
        /* A six-byte UTF-8 sequence. Make sure the other bytes are
           good. */
        if ((utf8_string[1] != '\0') &&
            (utf8_string[1] & 0xC0) != 0x80 &&
            (utf8_string[2] != '\0') &&
            (utf8_string[2] & 0xC0) != 0x80 &&
            (utf8_string[3] != '\0') &&
            (utf8_string[3] & 0xC0) != 0x80 &&
            (utf8_string[4] != '\0') &&
            (utf8_string[4] & 0xC0) != 0x80 &&
            (utf8_string[5] != '\0') && (utf8_string[5] & 0xC0) != 0x80) {
            return EILSEQ;
        }

        *ucs4_character =
            ((utf8_string[5] & 0x3F) << 0) +
            ((utf8_string[4] & 0x3F) << 6) +
            ((utf8_string[3] & 0x3F) << 12) +
            ((utf8_string[2] & 0x3F) << 18) +
            ((utf8_string[1] & 0x3F) << 24) + ((utf8_character & 0x01) << 30);
        *length = 6;
    }
    else {
        return EILSEQ;
    }

    return 0;
}

/** Convert a UTF-8 encoded character to CP437. */
int
unicode_utf8_to_cp437(unsigned char *cp_character, const utf8_t * utf8_string,
    size_t * length)
{
    ucs4_t ucs4_character;
    int result;

    result = unicode_utf8_to_ucs4(&ucs4_character, utf8_string, length);
    if (result != 0) {
        /* pass decoding characters upstream */
        return result;
    }

    if (ucs4_character < 0x7F) {
        *cp_character = (unsigned char)ucs4_character;
    }
    else {
        struct {
            ucs4_t ucs4;
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
int unicode_utf8_to_ascii(unsigned char *cp_character, const utf8_t * utf8_string,
    size_t *length)
{
    int result = unicode_utf8_to_cp437(cp_character, utf8_string, length);
    if (result == 0) {
        if (*length > 1) {
            *cp_character = '?';
        }
    }
    return result;
}

/** Convert a UTF-8 encoded character to CP1252. */
int unicode_utf8_to_cp1252(unsigned char *cp_character, const utf8_t * utf8_string,
    size_t * length)
{
    ucs4_t ucs4_character;
    int result;

    result = unicode_utf8_to_ucs4(&ucs4_character, utf8_string, length);
    if (result != 0) {
        /* pass decoding characters upstream */
        return result;
    }

    if (ucs4_character <= 0x7F || ucs4_character >= 0xA0) {
        *cp_character = (char)ucs4_character;
    }
    else {
        struct {
            ucs4_t ucs4;
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
