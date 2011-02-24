/*
Copyright (c) 2010-2011, Enno Rehling <enno@eressea.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include "quicklist.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define QL_MAXSIZE 14 /* total struct is 64 bytes */
#define QL_LIMIT 8

typedef struct quicklist {
  struct quicklist * next;
  int num_elements;
  void * elements[QL_MAXSIZE];
} quicklist;


void * ql_get(quicklist * ql, int index) {
  return (ql && index<ql->num_elements)?ql->elements[index]:ql_get(ql->next, index-ql->num_elements);
}

int ql_length(const quicklist * ql) {
  return ql?ql->num_elements+ql_length(ql->next):0;
}

void ql_push(quicklist ** qlp, void * data) {
  quicklist * ql = 0;
  while(*qlp && ((*qlp)->next || (*qlp)->num_elements==QL_MAXSIZE)) {
    qlp = &(*qlp)->next;
  }
  if (!*qlp) {
    ql = malloc(sizeof(quicklist));
    ql->num_elements = 0;
    ql->next = 0;
    *qlp = ql;
  } else {
    ql = *qlp;
  }
  ql->elements[ql->num_elements++] = data;
}

int ql_delete(quicklist ** qlp, int index) {
  quicklist * ql = *qlp;
  if (index<0) return EINVAL;
  if (ql && index>=ql->num_elements) {
    return ql_delete(&ql->next, index-ql->num_elements);
  } else if (ql) {
    if (index+1<ql->num_elements) {
      memmove(ql->elements+index, ql->elements+index+1, (ql->num_elements-index-1)*sizeof(void*));
    }
    --ql->num_elements;
    if (ql->num_elements==0) {
      *qlp = ql->next;
      free(ql);
    } else if (ql->next && ql->num_elements<QL_LIMIT) {
      quicklist * qn = ql->next;
      if (ql->num_elements+qn->num_elements>QL_MAXSIZE) {
        memcpy(ql->elements+ql->num_elements, qn->elements, sizeof(void*));
        --qn->num_elements;
        ++ql->num_elements;
        memmove(qn->elements, qn->elements+1, qn->num_elements*sizeof(void*));
      } else {
        memcpy(ql->elements+ql->num_elements, qn->elements, qn->num_elements*sizeof(void*));
        ql->num_elements += qn->num_elements;
        ql->next = qn->next;
        free(qn);
      }
    }
  }
  return 0;
}

int ql_insert(quicklist ** qlp, int index, void * data) {
  quicklist * ql = *qlp;
  if (ql) {
    if (index>=QL_MAXSIZE) {
      return ql_insert(&ql->next, index-ql->num_elements, data);
    } else if (ql->num_elements<QL_MAXSIZE) {
      memmove(ql->elements+index+1, ql->elements+index, (ql->num_elements-index)*sizeof(void*));
      ql->elements[index]=data;
      ++ql->num_elements;
    } else {
      quicklist * qn = malloc(sizeof(quicklist));
      qn->next = ql->next;
      ql->next = qn;
      qn->num_elements = QL_LIMIT;
      ql->num_elements -= QL_LIMIT;
      memcpy(qn->elements, ql->elements+ql->num_elements-QL_LIMIT, QL_LIMIT*sizeof(void*));
      if (index<=ql->num_elements) {
        return ql_insert(qlp, index, data);
      } else {
        return ql_insert(&ql->next, index-ql->num_elements, data);
      }
    }
  } else if (index==0) {
    ql_push(qlp, data);
  } else {
    return EINVAL;
  }
  return 0;
}
