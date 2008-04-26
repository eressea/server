#ifndef STORAGE_H
#define STORAGE_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct storage {
  /* separator for readable files: */
  int (*w_brk)(struct storage *);
  /* integer values: */
  int (*w_int)(struct storage *, int arg);
  int (*r_int)(struct storage *);
  /* float values: */
  int (*w_flt)(struct storage *, float arg);
  float (*r_flt)(struct storage *);
  /* id values: */
  int (*w_id)(struct storage *, int arg);
  int (*r_id)(struct storage *);
  /* tokens that contain no whitespace: */
  int (*w_tok)(struct storage *, const char * tok);
  char *(*r_tok)(struct storage *);
  void (*r_tok_buf)(struct storage *, char * result, size_t size);
  /* strings that need to be quoted: */
  int (*w_str)(struct storage *, const char * tok);
  char * (*r_str)(struct storage *);
  void (*r_str_buf)(struct storage *, char * result, size_t size);

  int (*open)(struct storage *, const char * filename, int mode);
  int (*close)(struct storage *);

  int encoding;
  int version;
  void * userdata;
} storage;

#ifdef __cplusplus
}
#endif
#endif
