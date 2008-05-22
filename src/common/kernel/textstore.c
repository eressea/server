/* vi: set ts=2:
+-------------------+  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
+-------------------+  
This program may not be used, modified or distributed 
without prior permission by the authors of Eressea.
*/
#include <config.h>
#include "eressea.h"
#include "textstore.h"

#include "save.h"
#include "version.h"
#include <util/base36.h>
#include <util/log.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <libxml/encoding.h>

#define NULL_TOKEN '@'

static int 
txt_w_brk(struct storage * store)
{
  putc('\n', (FILE *)store->userdata);
  return 1;
}

static int 
txt_w_id(struct storage * store, int arg)
{
  return fprintf((FILE *)store->userdata, "%s ", itoa36(arg));
}

static int
txt_r_id(struct storage * store)
{
  char id[8];
  fscanf((FILE *)store->userdata, "%7s", id);
  return atoi36(id);
}

static int 
txt_w_int(struct storage * store, int arg)
{
  return fprintf((FILE *)store->userdata, "%d ", arg);
}

static int
txt_r_int(struct storage * store)
{
  int result;
  fscanf((FILE *)store->userdata, "%d", &result);
  return result;
}

static int 
txt_w_flt(struct storage * store, float arg)
{
  return fprintf((FILE *)store->userdata, "%f ", arg);
}

static float
txt_r_flt(struct storage * store)
{
  double result;
  fscanf((FILE *)store->userdata, "%lf", &result);
  return (float)result;
}

static int
txt_w_tok(struct storage * store, const char * tok)
{
  int result;
  if (tok==NULL || tok[0]==0) {
    result = fputc(NULL_TOKEN, (FILE *)store->userdata);
  } else {
#ifndef NDEBUG
    const char * find = strchr(tok, ' ');
    if (!find) find = strchr(tok, NULL_TOKEN);
    assert(!find || !"reserved character in token");
#endif
    assert(tok[0]!=' ');
    result = fputs(tok, (FILE *)store->userdata);
  }
  fputc(' ', (FILE *)store->userdata);
  return result;
}
static char *
txt_r_tok(struct storage * store)
{
  char result[256];
  fscanf((FILE *)store->userdata, "%256s", result);
  if (result[0]==NULL_TOKEN || result[0]==0) {
    return NULL;
  }
  return strdup(result);
}
static void
txt_r_tok_buf(struct storage * store, char * result, size_t size)
{
  char format[16];
  format[0]='%';
  sprintf(format+1, "%us", size);
  fscanf((FILE *)store->userdata, format, result);
  if (result[0]==NULL_TOKEN) {
    result[0] = 0;
  }
}

static int
txt_w_str(struct storage * store, const char * str)
{
  int result = fwritestr((FILE *)store->userdata, str);
  fputc(' ', (FILE *)store->userdata);
  return result+1;
}

static char *
txt_r_str(struct storage * store)
{
  char buffer[DISPLAYSIZE];
  /* you should not use this */
  freadstr((FILE *)store->userdata, store->encoding, buffer, sizeof(buffer));
  return strdup(buffer);
}

static void
txt_r_str_buf(struct storage * store, char * result, size_t size)
{
  freadstr((FILE *)store->userdata, store->encoding, result, size);
}

static int
txt_open(struct storage * store, const char * filename, int mode)
{
  const char * modes[] = { 0, "rt", "wt", "at" };
  FILE * F = fopen(filename, modes[mode]);
  store->userdata = F;
  if (F) {
    const char utf8_bom[4] = { 0xef, 0xbb, 0xbf };
    if (mode==IO_READ) {
      char token[8];
      /* recognize UTF8 BOM */
      store->r_tok_buf(store, token, sizeof(token));
      if (memcmp(token, utf8_bom, 3)==0) {
        if (enc_gamedata!=XML_CHAR_ENCODING_UTF8) {
          log_warning(("Found UTF-8 BOM, assuming unicode game data.\n"));
          store->encoding = XML_CHAR_ENCODING_UTF8;
        }
        store->version = atoi(token+3);
      } else {
        if (store->encoding==XML_CHAR_ENCODING_NONE) {
          store->encoding = XML_CHAR_ENCODING_8859_1;
          log_warning(("No BOM, assuming 8859-1 game data.\n"));
        }
        store->version = atoi(token);
      }
    } else if (store->encoding==XML_CHAR_ENCODING_UTF8) {
      fputs(utf8_bom, F);
      fprintf(F, "%d\n", RELEASE_VERSION);
    }
  }
  return (F==NULL);
}

static int
txt_close(struct storage * store)
{
  return fclose((FILE *)store->userdata);
}

const storage text_store = {
  txt_w_brk,
  txt_w_int, txt_r_int,
  txt_w_flt, txt_r_flt,
  txt_w_id, txt_r_id,
  txt_w_tok, txt_r_tok, txt_r_tok_buf,
  txt_w_str, txt_r_str, txt_r_str_buf,
  txt_open, txt_close,
  0, 0, NULL
};

