#include <config.h>
#include "filereader.h"

#include <util/log.h>
#include <util/unicode.h>

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
#ifdef USE_UNICODE

  while (*ptr) {
    wint_t ucs;
    size_t size = 0;
    ret = unicode_utf8_to_ucs4(&ucs, (const xmlChar*)ptr, &size);
    if (ret!=0) break;
    if (!iswspace(ucs)) break;
    *total_size += size;
    ptr += size;
  }
#else 
  *total_size = 0;
  while (ptr[*total_size] && isspace(*(unsigned char*)ptr[*total_size])) ++*total_size;
#endif
  return ret;
}

const xmlChar *
getbuf(FILE * F, int encoding)
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
        bp = NULL;
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

      ret = unicode_utf8_to_ucs4(&ucs, (const xmlChar *)bp, &size);
      
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
  return (const xmlChar*)fbuf;
}
