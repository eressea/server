/* vi: set ts=2:
*
*	
* Eressea PB(E)M host Copyright (C) 1998-2003
*      Christian Schlittchen (corwin@amber.kn-bremen.de)
*      Katja Zedel (katze@felidae.kn-bremen.de)
*      Henning Peters (faroul@beyond.kn-bremen.de)
*      Enno Rehling (enno@eressea.de)
*      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
*
* based on:
*
*      Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
*      Atlantis v1.7                    Copyright 1996 by Alex Schröder
*
* This program may not be used, modified or distributed without
* prior permission by the authors of Eressea.
* This program may not be sold or used commercially without prior written
* permission from the authors.
*/

#include <config.h>
#include "umlaut.h"

#include "log.h"
#include "unicode.h"

#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct tref {
  struct tref * nexthash;
  ucs4_t ucs;
  struct tnode * node;
} tref;

#define LEAF 1 /* leaf node for a word. always matches */
#define SHARED 2 /* at least two words share the node */

void
addtoken(tnode * root, const char * str, variant id)
{
  static const struct replace { /* STATIC_CONST: constant value */
    ucs4_t ucs;
    const char str[3];
  } replace[] = {
    /* match lower-case (!) umlauts and others to transcriptions */
    { 228, "AE"}, /* auml */
    { 246, "OE"}, /* ouml */
    { 252, "UE"}, /* uuml */
    { 223, "SS"}, /* szlig */
    { 230, "AE"}, /* norsk */
    { 248, "OE"}, /* norsk */
    { 229, "AA"}, /* norsk */
    { 0, "" }
  };

  if (!*str) {
    root->id = id;
    root->flags |= LEAF;
  } else {
    tref * next;
    int ret, index, i = 0;
    ucs4_t ucs, lcs;
    size_t len;

    ret = unicode_utf8_to_ucs4(&ucs, str, &len);
    assert(ret==0 || !"invalid utf8 string");
    lcs = ucs;

#if NODEHASHSIZE == 8
    index = ucs & 7;
#else
    index = ucs % NODEHASHSIZE;
#endif
    assert(index>=0);
    next = root->next[index];
    if (!(root->flags & LEAF)) root->id = id;
    while (next && next->ucs != ucs) next = next->nexthash;
    if (!next) {
      tref * ref;
      tnode * node = calloc(1, sizeof(tnode));

      if (ucs<'a' || ucs>'z') {
        lcs = towlower((wint_t)ucs);
      }
      if (ucs==lcs) {
        ucs = towupper((wint_t)ucs);
      }

      ref = malloc(sizeof(tref));
      ref->ucs = ucs;
      ref->node = node;
      ref->nexthash=root->next[index];
      root->next[index] = ref;

      /* try lower/upper casing the character, and try again */
      if (ucs!=lcs) {
#if NODEHASHSIZE == 8
        index = lcs & 7;
#else
        index = lcs % NODEHASHSIZE;
#endif
        ref = malloc(sizeof(tref));
        ref->ucs = lcs;
        ref->node = node;
        ref->nexthash = root->next[index];
        root->next[index] = ref;
      }
      next=ref;
    } else {
      next->node->flags |= SHARED;
      if ((next->node->flags & LEAF) == 0) next->node->id.v = NULL; /* why?*/
    }
    addtoken(next->node, str+len, id);
    while (replace[i].str[0]) {
      if (lcs==replace[i].ucs) {
        char zText[1024];
        memcpy(zText, replace[i].str, 3);
        strcpy(zText+2, (const char*)str+len);
        addtoken(root, zText, id);
        break;
      }
      ++i;
    }
  }
}

int
findtoken(const tnode * tk, const char * str, variant* result)
{
  if (!str || *str==0) return E_TOK_NOMATCH;

  do {
    int index;
    const tref * ref;
    ucs4_t ucs;
    size_t len;
    int ret = unicode_utf8_to_ucs4(&ucs, str, &len);

    if (ret!=0) {
      /* encoding is broken. youch */
      return E_TOK_NOMATCH;
    }
#if NODEHASHSIZE == 8
    index = ucs & 7;
#else
    index = ucs % NODEHASHSIZE;
#endif
    ref = tk->next[index];
    while (ref && ref->ucs!=ucs) ref = ref->nexthash;
    str+=len;
    if (!ref) return E_TOK_NOMATCH;
    tk = ref->node;
  } while (*str);
  if (tk) {
    *result = tk->id;
    return E_TOK_SUCCESS;
  }
  return E_TOK_NOMATCH;
}
