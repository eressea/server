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
static int 
bin_w_brk(struct storage * store)
{
  return 0;
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
    static const int empty = 0;
    result = (int)fwrite(&empty, sizeof(int), 1, file(store));
  } else {
    int size = (int)strlen(tok);
    result = (int)fwrite(&size, sizeof(int), 1, file(store));
    result += (int)fwrite(tok, size, 1, file(store));
  }
  return result;
}
static char *
bin_r_str(struct storage * store)
{
  int len;

  fread(&len, sizeof(len), 1, file(store));
  if (len) {
    char * result = malloc(len+1);

    fread(result, sizeof(char), len, file(store));
    result[len] = 0;
    return result;
  }
  return NULL;
}
static void
bin_r_str_buf(struct storage * store, char * result, size_t size)
{
  int i;
  size_t rd, len;

  fread(&i, sizeof(int), 1, file(store));
  assert(i>=0);
  if (i==0) {
    result[0] = 0;
  } else {
    len = (size_t)i;
    rd = min(len, size);
    fread(result, sizeof(char), rd, file(store));
    if (rd<len) {
      fseek(file(store), (long)(len-rd), SEEK_CUR);
      result[size-1] = 0;
    } else {
      result[len] = 0;
    }
  }
}

int
bin_open(struct storage * store, const char * filename, int mode)
{
  const char * modes[] = { 0, "rb", "wb", "ab" };
  FILE * F = fopen(filename, modes[mode]);
  store->userdata = F;
  store->encoding=XML_CHAR_ENCODING_UTF8; /* always utf8 it is */
  if (F) {
    if (mode==IO_READ) {
      store->version = store->r_int(store);
    } else if (store->encoding==XML_CHAR_ENCODING_UTF8) {
      store->w_int(store, RELEASE_VERSION);
    }
  }
  return (F==NULL);
}

int
bin_close(struct storage * store)
{
  return fclose(file(store));
}

const storage binary_store = {
  bin_w_brk,
  bin_w_int, bin_r_int,
  bin_w_flt, bin_r_flt,
  bin_w_int, bin_r_int,
  bin_w_str, bin_r_str, bin_r_str_buf,
  bin_w_str, bin_r_str, bin_r_str_buf,
  bin_open, bin_close,
  0, 0, NULL
};

