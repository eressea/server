#include <config.h>
#include "filereader.h"

#include <util/log.h>
#include <util/unicode.h>

#include <libxml/encoding.h>

#include <ctype.h>
#include <wctype.h>

#define COMMENT_CHAR    ';'
#define CONTINUE_CHAR    '\\'
#define MAXLINE 4096*16
static char lbuf[MAXLINE];
static char fbuf[MAXLINE];

static void
unicode_warning(const char * bp)
{
  log_warning(("invalid sequence in UTF-8 string: %s\n", bp));
}

INLINE_FUNCTION int
eatwhite(const char * ptr, size_t * total_size)
{
  int ret = 0;

  *total_size = 0;

  while (*ptr) {
    wint_t ucs;
    size_t size = 0;
    ret = unicode_utf8_to_ucs4(&ucs, ptr, &size);
    if (ret!=0) break;
    if (!iswspace(ucs)) break;
    *total_size += size;
    ptr += size;
  }
  return ret;
}

static const char *
getbuf_latin1(FILE * F)
{
  boolean cont = false;
  char quote = 0;
  boolean comment = false;
  char * cp = fbuf;
  char * tail = lbuf+MAXLINE-2;

  tail[1] = '@'; /* if this gets overwritten by fgets then the line was very long. */
  do {
    const char * bp = fgets(lbuf, MAXLINE, F);

    if (bp==NULL) return NULL;
    while (*bp && isspace(*(unsigned char*)bp)) ++bp; /* eatwhite */

    comment = (boolean)(comment && cont);

    if (tail[1]==0) {
      /* we read he maximum number of bytes! */
      if (tail[0]!='\n') {
        /* it wasn't enough space to finish the line, eat the rest */
        for (;;) {
          tail[1] = '@';
          bp = fgets(lbuf, MAXLINE, F);
          if (bp==NULL) return NULL;
          if (tail[1]) {
            /* read enough this time to end the line */
            break;
          }
        }
        comment = false;
        cont = false;
        bp = NULL;
        continue;
      } else {
        tail[1] = '@';
      }
    }
    cont = false;
    while (*bp && cp<fbuf+MAXLINE) {
      int c = *(unsigned char *)bp;
      
      if (c==COMMENT_CHAR && !quote) {
        /* comment begins. we need to keep going, to look for CONTINUE_CHAR */
        comment = true;
        ++bp;
        continue;
      }
      if (c=='"' || c=='\'') {
        if (quote==c) {
          quote = 0;
          if (cp<fbuf+MAXLINE) *cp++ = *bp;
          ++bp;
          continue;
        } else if (!quote) {
          quote = *bp++;
          if (cp<fbuf+MAXLINE) *cp++ = quote;
          continue;
        }
      }

      if (isspace(c)) {
        if (!quote) {
          ++bp;
          while (*bp && isspace(*(unsigned char*)bp)) ++bp; /* eatwhite */
          if (!comment && *bp && *bp!=COMMENT_CHAR && cp<fbuf+MAXLINE) *(cp++) = ' ';
        }
        else if (!comment && cp+1<=fbuf+MAXLINE) {
          *(cp++)=*(bp++);
        } else {
          ++bp;
        }
        continue;
      } else if (iscntrl(c)) {
        if (!comment && cp<fbuf+MAXLINE) *(cp++) = '?';
        ++bp;
        continue;
      } else if (c==CONTINUE_CHAR) {
        const char * end = ++bp;
        while (*end && isspace(*(unsigned char*)end)) ++end; /* eatwhite */
        if (*end == '\0') {
          bp = end;
          cont = true;
          continue;
        }
        if (comment) {
          ++bp;
          continue;
        }
      } else if (comment) {
        ++bp;
        continue;
      }

      if (c < 0x80) {
        if (cp+1<=fbuf+MAXLINE) {
          *(cp++)=*(bp++);
        }
      } else {
        char inbuf = (char)c;
        int inbytes = 1;
        int outbytes = (int)(MAXLINE-(cp-fbuf));
        int ret = isolat1ToUTF8((xmlChar *)cp, &outbytes, (const xmlChar *)&inbuf, &inbytes);
        if (ret>0) cp+=ret;
        ++bp;
        continue;
      }
    }
    if (cp==fbuf+MAXLINE) {
      --cp;
    }
    *cp=0;
  } while (cont || cp==fbuf);
  return fbuf;
}

static const char *
getbuf_utf8(FILE * F)
{
  boolean cont = false;
  char quote = 0;
  boolean comment = false;
  char * cp = fbuf;
  char * tail = lbuf+MAXLINE-2;

  tail[1] = '@'; /* if this gets overwritten by fgets then the line was very long. */
  do {
    const char * bp = fgets(lbuf, MAXLINE, F);
    size_t white;
    if (bp==NULL) return NULL;

    eatwhite(bp, &white); /* decoding errors will get caught later on, don't have to check */
    bp += white;

    comment = (boolean)(comment && cont);

    if (tail[1]==0) {
      /* we read he maximum number of bytes! */
      if (tail[0]!='\n') {
        /* it wasn't enough space to finish the line, eat the rest */
        for (;;) {
          tail[1] = '@';
          bp = fgets(lbuf, MAXLINE, F);
          if (bp==NULL) return NULL;
          if (tail[1]) {
            /* read enough this time to end the line */
            break;
          }
        }
        comment = false;
        cont = false;
        bp = NULL;
        continue;
      } else {
        tail[1] = '@';
      }
    }
    cont = false;
    while (*bp && cp<fbuf+MAXLINE) {
      wint_t ucs;
      size_t size;
      int ret;
      
      if (*bp==COMMENT_CHAR && !quote) {
        /* comment begins. we need to keep going, to look for CONTINUE_CHAR */
        comment = true;
        ++bp;
      }
      if (*bp=='"' || *bp=='\'') {
        if (quote==*bp) {
          quote = 0;
          if (cp<fbuf+MAXLINE) *cp++ = *bp++;
        } else if (!quote) {
          quote = *bp++;
          if (cp<fbuf+MAXLINE) *cp++ = quote;
        }
      }

      ret = unicode_utf8_to_ucs4(&ucs, bp, &size);
      
      if (ret!=0) {
        unicode_warning(bp);
        break;
      }

      if (iswspace(ucs)) {
        if (!quote) {
          bp+=size;
          ret = eatwhite(bp, &size);
          if (!comment && *bp && *bp!=COMMENT_CHAR && cp<fbuf+MAXLINE) *(cp++) = ' ';
          bp += size;
          if (ret!=0) {
            unicode_warning(bp);
            break;
          }
        }
        else if (!comment) {
          if (cp+size<=fbuf+MAXLINE) {
            while (size--) {
              *(cp++)=*(bp++);
            }
          } else bp+=size;
        } else {
          bp+=size;
        }
      }
      else if (iswcntrl(ucs)) {
        if (!comment && cp<fbuf+MAXLINE) *(cp++) = '?';
        bp+=size;
      } else {
        if (*bp==CONTINUE_CHAR) {
          const char * end;
          eatwhite(bp+1, &white);
          end = bp+1+white;
          if (*end == '\0') {
            bp = end;
            cont = true;
            continue;
          }
          if (!comment && cp<fbuf+MAXLINE) *cp++ = *bp++;
          else ++bp;
        } else {
          if (!comment && cp+size<=fbuf+MAXLINE) {
            while (size--) {
              *(cp++)=*(bp++);
            }
          } else {
            bp += size;
          }
        }
      }
    }
    if (cp==fbuf+MAXLINE) {
      --cp;
    }
    *cp=0;
  } while (cont || cp==fbuf);
  return fbuf;
}

const char *
getbuf(FILE * F, int encoding)
{
  if (encoding==XML_CHAR_ENCODING_UTF8) return getbuf_utf8(F);
  return getbuf_latin1(F);
}
