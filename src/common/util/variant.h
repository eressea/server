#ifndef STRUCT_VARIANT_H
#define STRUCT_VARIANT_H

typedef union {
	void *v;
	int   i;
	char  c;
	short s;
	short sa[2];
	char  ca[4];
} variant;

#define VAR(x) ()
#endif
