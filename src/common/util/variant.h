#ifndef STRUCT_VARIANT_H
#define STRUCT_VARIANT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef union {
	void *v;
	int   i;
	char  c;
	short s;
	short sa[2];
	char  ca[4];
} variant;

#define VAR(x) ()

#ifdef __cplusplus
}
#endif
#endif
