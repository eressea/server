#include <platform.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "patricia.h"

#define MAXKEYLEN 128

/* TODO: custom memory management to optimize cache layout, or use arrays. */

/* NOTE: The structure saves an extra 0 delimiter for the key. Technically 
 * this wouldn't be necessary (because we know its' length from data[0]), 
 * but it makes it possible for trie_getkey to return a key without making 
 * a copy or have a cumbersome (const char**, size_t*) interface.
 *       +-----------+-------------+------+------------+
 * data: | keylen(1) | key(keylen) | 0(1) | data(size) |
 *       +-----------+-------------+------+------------+
 */

struct trie_node {
  struct trie_node *l, *r;
  char * data;
  unsigned int bitpos;
};

#if 1
#define get_bit(c, s, p) (unsigned int)((((p)>>3)>(unsigned int)(s))?0:((c)[(p)>>3]>>((p)&7)&1))
#else
unsigned int get_bit(const char * c, size_t s, unsigned int p)
{
  if ((p>>3)>=(unsigned int)s) return 0;
  return ((c)[p>>3]>>(p&7)&1);
}
#endif
#define node_bit(n, p) get_bit((n)->data+1, (n)->data[0], (p))

trie_node * trie_insert(trie_node **root_p, const char * key, const void * data, size_t size)
{
  trie_node * new_node;
  size_t keylen = strlen(key);
  trie_node ** insert_p = root_p, *node = *insert_p;
  unsigned int p, bit=0;

  assert(keylen<MAXKEYLEN);

  for (p=0;p!=keylen*8+1;++p) {
    bit = get_bit(key, keylen, p);

    /* NULL-pointers lead to someplace we haven't got a prefix yet. */
    if (node==NULL) {
      break;
    }

    /* if we have the full prefix that the current node represents, move on */
    if (p==node->bitpos) {
      insert_p = bit?&node->r:&node->l;
      node = *insert_p;
      if (node==NULL) {
        continue;
      }
    }

    /* if we are looking at a back-node, we need to add our node before it. */
    if (p>=node->bitpos) {
      /* find the point p where both differ. */
      if (keylen==(unsigned int)node->data[0] && strncmp(key, node->data+1, keylen)==0) {
        /* we are trying to insert the same key again */

        return node;
      }
      do {
        ++p;
        bit = get_bit(key, keylen, p);
      } while (node_bit(node, p)==bit);
      break;
    }

    /* if instead we differ before reaching the end of the current prefix, we must split.
     * we insert our node before the current one and re-attach it. */
    if (node_bit(node, p)!=bit) {
      break;
    }
  }

  new_node = (trie_node *)malloc(sizeof(trie_node));
  new_node->bitpos = p;
  new_node->data = malloc(keylen+2+size);
  new_node->data[0] = (char)keylen;
  memcpy(new_node->data+1, key, keylen+1);
  if (data!=NULL && size>0) {
    /* if data is NULL then the user only wanted some space that they're going to write to later */
    /* if size is 0 then the user is using the trie as a set, not a map */
    memcpy(new_node->data+2+keylen, data, size);
  }
  if (bit) {
    new_node->l = node;
    new_node->r = new_node; /* loop the 1-bit to ourselves, search will end */
  } else {
    new_node->l = new_node; /* loop the 0-bit to ourselves, search will end */
    new_node->r = node;
  }
  *insert_p = new_node;
  return new_node;
}

void trie_remove(trie_node **root_p, trie_node *pos)
{
  if (pos!=NULL) {
    const char * key = trie_getkey(pos);
    size_t keylen = pos->data[0];
    trie_node ** node_p = root_p;
    trie_node * node = *root_p;

    while (node) {
      int bit;
      trie_node ** next_p;
      trie_node * next;

      if (node == pos) {
        if (node->l==node) {
          *node_p = node->r;
          break;
        } else if (node->r==node) {
          *node_p = node->l;
          break;
        }
      }
      
      bit = get_bit(key, keylen, node->bitpos);
      next_p = bit?&node->r:&node->l;
      next = *next_p;
      if (next == pos && next->bitpos<=node->bitpos) {
        /* the element that has a back-pointer to pos gets swapped with pos */
        char * data = pos->data;
        pos->data = node->data;
        node->data = data;

        /* finally, find the back-pointer to node and set it to pos */
        next_p = bit?&node->l:&node->r; /* NB: this is the OTHER child of node */
        next = *next_p;
        key = trie_getkey(node);
        keylen = (unsigned int)node->data[0];
        while (next) {
          int new_bit;
          if (next==node) {
            *next_p = pos;
            break;
          }
          new_bit = get_bit(key, keylen, next->bitpos);
          next_p = new_bit?&next->r:&next->l;
          next = *next_p;
        }
        *node_p = bit?node->l:node->r;
        break;
      }
      node = *next_p;
      node_p = next_p;
    }
    free(node->data);
    free(node);
  }
}

void trie_debug(trie_node * root)
{
  const char * l = root->l?trie_getkey(root->l):"?";
  const char * r = root->r?trie_getkey(root->r):"?";
  printf("%s %d | %s | %s\n", trie_getkey(root), root->bitpos, l, r);
  if (root->l && root->l->bitpos > root->bitpos) trie_debug(root->l);
  if (root->r && root->r->bitpos > root->bitpos) trie_debug(root->r);
}

trie_node * trie_find(trie_node *root, const char *key)
{
  trie_node * node = root;
  size_t keylen = strlen(key);

  while (node) {
    int bit = get_bit(key, keylen, node->bitpos);
    trie_node * next = bit?node->r:node->l;

    if (next!=NULL) {
      if (node->bitpos>=next->bitpos) {
        if (keylen==(unsigned int)next->data[0] && strncmp(key, next->data+1, keylen)==0) {
          return next;
        }
        next = NULL;
      }
    }
    node = next;
  }
  return NULL;
}

trie_node * trie_find_prefix(trie_node *root, const char *key)
{
  trie_node * node = root;
  size_t keylen = strlen(key);

  while (node) {
    int bit = get_bit(key, keylen, node->bitpos);
    trie_node * next = bit?node->r:node->l;

    if (next!=NULL) {
      if (node->bitpos>=next->bitpos) {
        if (keylen<=(unsigned int)next->data[0] && strncmp(key, next->data+1, keylen)==0) {
          return next;
        }
        next = NULL;
      }
    }
    node = next;
  }
  return NULL;
}

void * trie_getdata(trie_node * node)
{
  return node->data+2+node->data[0];
}

const char * trie_getkey(trie_node * node)
{
  return node->data+1;
}

void trie_free(trie_node * root)
{
  if (root) {
    if (root->l && root->l->bitpos>root->bitpos) trie_free(root->l);
    if (root->r && root->r->bitpos>root->bitpos) trie_free(root->r);
    free(root);
  }
}
