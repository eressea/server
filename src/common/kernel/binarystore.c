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

#define file(store) (FILE *)((store)->userdata)

#define STREAM_VERSION 2

INLINE_FUNCTION size_t
pack_int(int v, char * buffer)
{
  int sign = (v<0);

  if (sign) {
    v = ~v + 1;
    sign = 0x40;
  }
  if (v<0x40) {
    buffer[0] = (char)(v | sign);
    return 1;
  } else if (v<0x2000) {
    buffer[0] = (char)((v>> 6) | 0x80);
    buffer[1] = (char)((v & 0x3F) | sign);
    return 2;
  } else if (v<0x100000) {
    buffer[0] = (char)(((v>>13) & 0x7f) | 0x80);
    buffer[1] = (char)(((v>> 6) & 0x7f) | 0x80);
    buffer[2] = (char)((v & 0x3F) | sign);
    return 3;
  } else if (v<0x8000000) {
    buffer[0] = (char)(((v>>20) & 0x7f) | 0x80);
    buffer[1] = (char)(((v>>13) & 0x7f) | 0x80);
    buffer[2] = (char)(((v>> 6) & 0x7f) | 0x80);
    buffer[3] = (char)((v & 0x3F) | sign);
    return 4;
  }
  buffer[0] = (char)(((v>>27) & 0x7f) | 0x80);
  buffer[1] = (char)(((v>>20) & 0x7f) | 0x80);
  buffer[2] = (char)(((v>>13) & 0x7f) | 0x80);
  buffer[3] = (char)(((v>> 6) & 0x7f) | 0x80);
  buffer[4] = (char)((v & 0x3F) | sign);
  return 5;
}

INLINE_FUNCTION int
unpack_int(const char * buffer)
{
  int i = 0, v = 0;

  while (buffer[i] & 0x80) {
    v = (v << 7) | (buffer[i++] & 0x7f);
  }
  v = (v << 6) | (buffer[i] & 0x3f);

  if (buffer[i] & 0x40) {
    v = ~v + 1;
  }
  return v;
}

static int 
bin_w_brk(struct storage * store)
{
  return 0;
}

static int 
bin_w_int_pak(struct storage * store, int arg)
{
  char buffer[5];
  size_t size = pack_int(arg, buffer);
  return (int)fwrite(buffer, sizeof(char), size, file(store));
}

static int
bin_r_int_pak(struct storage * store)
{
  int v = 0;
  char ch;

  fread(&ch, sizeof(char), 1, file(store));
  while (ch & 0x80) {
    v = (v << 7) | (ch & 0x7f);
    fread(&ch, sizeof(char), 1, file(store));
  }
  v = (v << 6) | (ch & 0x3f);

  if (ch & 0x40) {
    v = ~v + 1;
  }
  return v;
}

static int 
bin_w_int(struct storage * store, int arg)
{
  return (int)fwrite(&arg, sizeof(arg), 1, file(store));
}

static int
bin_r_int(struct storage * store)
{
  int result;
  fread(&result, sizeof(result), 1, file(store));
  return result;
}

static int 
bin_w_flt(struct storage * store, float arg)
{
  return (int)fwrite(&arg, sizeof(arg), 1, file(store));
}

static float
bin_r_flt(struct storage * store)
{
  float result;
  fread(&result, sizeof(result), 1, file(store));
  return result;
}

static int
bin_w_str(struct storage * store, const char * tok)
{
  int result;
  if (tok==NULL || tok[0]==0) {
    result = store->w_int(store, 0);
  } else {
    int size = (int)strlen(tok);
    result = store->w_int(store, size);
    result += (int)fwrite(tok, size, 1, file(store));
  }
  return result;
}

#define FIX_INVALID_CHARS /* required for data pre-574 */
static char *
bin_r_str(struct storage * store)
{
  int len;

  len = store->r_int(store);
  if (len) {
    char * result = malloc(len+1);

    fread(result, sizeof(char), len, file(store));
    result[len] = 0;
#ifdef FIX_INVALID_CHARS
    {
      char * p = strpbrk(result, "\n\r");
      while (p) {
        log_error(("Invalid character %d in input string \"%s\".\n", *p, result));
        strcpy(p, p+1);
        p = strpbrk(p, "\n\r");
      }
    }
#endif
    return result;
  }
  return NULL;
}

static void
bin_r_str_buf(struct storage * store, char * result, size_t size)
{
  int i;
  size_t rd, len;

  i = store->r_int(store);
  assert(i>=0);
  if (i==0) {
    result[0] = 0;
  } else {
    len = (size_t)i;
    rd = MIN(len, size-1);
    fread(result, sizeof(char), rd, file(store));
    if (rd<len) {
      fseek(file(store), (long)(len-rd), SEEK_CUR);
      result[size-1] = 0;
    } else {
      result[len] = 0;
    }
#ifdef FIX_INVALID_CHARS
    {
      char * p = strpbrk(result, "\n\r");
      while (p) {
        log_error(("Invalid character %d in input string \"%s\".\n", *p, result));
        strcpy(p, p+1);
        p = strpbrk(p, "\n\r");
      }
    }
#endif
  }
}

static int
bin_open(struct storage * store, const char * filename, int mode)
{
  const char * modes[] = { 0, "rb", "wb", "ab" };
  FILE * F = fopen(filename, modes[mode]);
  store->userdata = F;
  store->encoding=XML_CHAR_ENCODING_UTF8; /* always utf8 it is */
  if (F) {
    if (mode==IO_READ) {
      int stream_version = 0;
      store->version = bin_r_int(store);
      if (store->version>=INTPAK_VERSION) {
        stream_version = bin_r_int(store);
      }
      if (stream_version<=1) {
        store->r_id = bin_r_int;
        store->w_id = bin_w_int;
      }
      if (stream_version==0) {
        store->r_int = bin_r_int;
        store->w_int = bin_w_int;
      }
    } else if (store->encoding==XML_CHAR_ENCODING_UTF8) {
      bin_w_int(store, RELEASE_VERSION);
      bin_w_int(store, STREAM_VERSION);
    }
  }
  return (F==NULL);
}

static int
bin_close(struct storage * store)
{
  return fclose(file(store));
}

const storage binary_store = {
  bin_w_brk, /* newline (ignore) */
  bin_w_int_pak, bin_r_int_pak, /* int storage */
  bin_w_flt, bin_r_flt, /* float storage */
  bin_w_int_pak, bin_r_int_pak, /* id storage */
  bin_w_str, bin_r_str, bin_r_str_buf, /* token storage */
  bin_w_str, bin_r_str, bin_r_str_buf, /* string storage */
  bin_open, bin_close,
  0, 0, NULL
};

