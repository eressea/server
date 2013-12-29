/* vi: set ts=2:
+-------------------+  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
+-------------------+  
This program may not be used, modified or distributed 
without prior permission by the authors of Eressea.
*/
#include <platform.h>
#include "config.h"
#include "textstore.h"

#include "save.h"
#include "version.h"
#include <util/unicode.h>
#include <util/base36.h>
#include <util/log.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/encoding.h>

#define NULL_TOKEN '@'

/** writes a quoted string to the file
* no trailing space, since this is used to make the creport.
*/
int fwritestr(FILE * F, const char *str)
{
  int nwrite = 0;
  fputc('\"', F);
  if (str)
  while (*str) {
    int c = (int)(unsigned char)*str++;
    switch (c) {
    case '"':
    case '\\':
      fputc('\\', F);
      fputc(c, F);
      nwrite += 2;
      break;
    case '\n':
      fputc('\\', F);
      fputc('n', F);
      nwrite += 2;
      break;
    default:
      fputc(c, F);
      ++nwrite;
    }
  }
  fputc('\"', F);
  return nwrite + 2;
}

static int freadstr(FILE * F, int encoding, char *start, size_t size)
{
  char *str = start;
  bool quote = false;
  for (;;) {
    int c = fgetc(F);

    if (isxspace(c)) {
      if (str == start) {
        continue;
      }
      if (!quote) {
        *str = 0;
        return (int)(str - start);
      }
    }
    switch (c) {
    case EOF:
      return EOF;
    case '"':
      if (!quote && str != start) {
        log_error(
          ("datafile contains a \" that isn't at the start of a string.\n"));
        assert
          (!"datafile contains a \" that isn't at the start of a string.\n");
      }
      if (quote) {
        *str = 0;
        return (int)(str - start);
      }
      quote = true;
      break;
    case '\\':
      c = fgetc(F);
      switch (c) {
      case EOF:
        return EOF;
      case 'n':
        if ((size_t)(str - start + 1) < size) {
          *str++ = '\n';
        }
        break;
      default:
        if ((size_t)(str - start + 1) < size) {
          if (encoding == XML_CHAR_ENCODING_8859_1 && c & 0x80) {
            char inbuf = (char)c;
            size_t inbytes = 1;
            size_t outbytes = size - (str - start);
            int ret = unicode_latin1_to_utf8(str, &outbytes, &inbuf, &inbytes);
            if (ret > 0)
              str += ret;
            else {
              log_error("input data was not iso-8859-1! assuming utf-8\n");
              encoding = XML_CHAR_ENCODING_ERROR;
              *str++ = (char)c;
            }
          }
          else {
            *str++ = (char)c;
          }
        }
      }
      break;
    default:
      if ((size_t)(str - start + 1) < size) {
        if (encoding == XML_CHAR_ENCODING_8859_1 && c & 0x80) {
          char inbuf = (char)c;
          size_t inbytes = 1;
          size_t outbytes = size - (str - start);
          int ret = unicode_latin1_to_utf8(str, &outbytes, &inbuf, &inbytes);
          if (ret > 0)
            str += ret;
          else {
            log_error("input data was not iso-8859-1! assuming utf-8\n");
            encoding = XML_CHAR_ENCODING_ERROR;
            *str++ = (char)c;
          }
        }
        else {
          *str++ = (char)c;
        }
      }
    }
  }
}

static int txt_w_brk(struct storage *store)
{
  putc('\n', (FILE *) store->userdata);
  return 1;
}

static int txt_w_id(struct storage *store, int arg)
{
  return fprintf((FILE *) store->userdata, "%s ", itoa36(arg));
}

static int txt_r_id(struct storage *store)
{
  char id[8];
  fscanf((FILE *) store->userdata, "%7s", id);
  return atoi36(id);
}

static int txt_w_int(struct storage *store, int arg)
{
  return fprintf((FILE *) store->userdata, "%d ", arg);
}

static int txt_r_int(struct storage *store)
{
  int result;
  fscanf((FILE *) store->userdata, "%d", &result);
  return result;
}

static int txt_w_flt(struct storage *store, float arg)
{
  return fprintf((FILE *) store->userdata, "%f ", arg);
}

static float txt_r_flt(struct storage *store)
{
  double result;
  fscanf((FILE *) store->userdata, "%lf", &result);
  return (float)result;
}

static int txt_w_tok(struct storage *store, const char *tok)
{
  int result;
  if (tok == NULL || tok[0] == 0) {
    result = fputc(NULL_TOKEN, (FILE *) store->userdata);
  } else {
#ifndef NDEBUG
    const char *find = strchr(tok, ' ');
    if (!find)
      find = strchr(tok, NULL_TOKEN);
    assert(!find || !"reserved character in token");
#endif
    assert(tok[0] != ' ');
    result = fputs(tok, (FILE *) store->userdata);
  }
  fputc(' ', (FILE *) store->userdata);
  return result;
}

static char *txt_r_tok(struct storage *store)
{
  char result[256];
  fscanf((FILE *) store->userdata, "%256s", result);
  if (result[0] == NULL_TOKEN || result[0] == 0) {
    return NULL;
  }
  return _strdup(result);
}

static void txt_r_tok_buf(struct storage *store, char *result, size_t size)
{
  char format[16];
  if (result && size > 0) {
    format[0] = '%';
    sprintf(format + 1, "%lus", (unsigned long)size);
    fscanf((FILE *) store->userdata, format, result);
    if (result[0] == NULL_TOKEN) {
      result[0] = 0;
    }
  } else {
    /* trick to skip when no result expected */
    fscanf((FILE *) store->userdata, "%*s");
  }
}

static int txt_w_str(struct storage *store, const char *str)
{
  int result = fwritestr((FILE *) store->userdata, str);
  fputc(' ', (FILE *) store->userdata);
  return result + 1;
}

static char *txt_r_str(struct storage *store)
{
  char buffer[DISPLAYSIZE];
  /* you should not use this */
  freadstr((FILE *) store->userdata, store->encoding, buffer, sizeof(buffer));
  return _strdup(buffer);
}

static void txt_r_str_buf(struct storage *store, char *result, size_t size)
{
  freadstr((FILE *) store->userdata, store->encoding, result, size);
}

static int txt_open(struct storage *store, const char *filename, int mode)
{
  const char *modes[] = { 0, "rt", "wt", "at" };
  FILE *F = fopen(filename, modes[mode]);
  store->userdata = F;
  if (F) {
    const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf };
    if (mode == IO_READ) {
      char token[8];
      /* recognize UTF8 BOM */
      store->r_tok_buf(store, token, sizeof(token));
      if (memcmp(token, utf8_bom, 3) == 0) {
        if (enc_gamedata != XML_CHAR_ENCODING_UTF8) {
          log_warning("Found UTF-8 BOM, assuming unicode game data.\n");
          store->encoding = XML_CHAR_ENCODING_UTF8;
        }
        store->version = atoi(token + 3);
      } else {
        if (store->encoding == XML_CHAR_ENCODING_NONE) {
          store->encoding = XML_CHAR_ENCODING_8859_1;
          log_warning("No BOM, assuming 8859-1 game data.\n");
        }
        store->version = atoi(token);
      }
    } else if (store->encoding == XML_CHAR_ENCODING_UTF8) {
      fputs((const char *)utf8_bom, F);
      fprintf(F, "%d\n", RELEASE_VERSION);
    }
  }
  return (F == NULL);
}

static int txt_w_bin(struct storage *store, void *arg, size_t size)
{
  assert(!"not implemented!");
  return 0;
}

static void txt_r_bin(struct storage *store, void *result, size_t size)
{
  assert(!"not implemented!");
}

static int txt_close(struct storage *store)
{
  return fclose((FILE *) store->userdata);
}

const storage text_store = {
  txt_w_brk,
  txt_w_int, txt_r_int,
  txt_w_flt, txt_r_flt,
  txt_w_id, txt_r_id,
  txt_w_tok, txt_r_tok, txt_r_tok_buf,
  txt_w_str, txt_r_str, txt_r_str_buf,
  txt_w_bin, txt_r_bin,
  txt_open, txt_close,
  0, 0, NULL
};
