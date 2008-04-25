#ifndef H_PATRICIA
#define H_PATRICIA
#ifdef __cplusplus
extern "C" {
#endif

typedef struct trie_node trie_node;

trie_node * trie_insert(trie_node **root, const char *key, const void *data, size_t size);
trie_node * trie_find(trie_node *root, const char *key);
void * trie_getdata(trie_node *node);
const char * trie_getkey(trie_node *node);
void trie_free(trie_node * root);
void trie_remove(trie_node **root_p, trie_node *pos);
void trie_debug(trie_node * root);
trie_node * trie_find_prefix(trie_node *root, const char *key);

#ifdef __cplusplus
}
#endif
#endif
