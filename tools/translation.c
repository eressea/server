#include "util/pofile.h"
#include "util/strings.h"

#include <stb_ds.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct keyval_t {
    char * key;
    char * value;
} keyval_t;

const char *filter_ctx = NULL;

static int add_po_string(const char *msgid, const char *msgstr, const char *msgctx, void *data)
{
    if (!filter_ctx || (msgctx && strcmp(filter_ctx, msgctx) == 0))
    {
        keyval_t **hash_ref = (keyval_t**)data;
        keyval_t *hash = *hash_ref;
        shput(hash, str_strdup(msgid), strdup(msgstr));
        *hash_ref = hash;
    }
    return 0;
}

int main(int argc, char **argv)
{
    keyval_t *hm_de = NULL;
    keyval_t *hm_en = NULL;
    ptrdiff_t i;
    if (argc >= 1) {
        filter_ctx = argv[1];
    }
    pofile_read("res/translations/strings.de.po", add_po_string, &hm_de);
    pofile_read("res/translations/strings.en.po", add_po_string, &hm_en);
    for (i=shlen(hm_de);i != 0; --i) {
        keyval_t *de = hm_de + i - 1;
        keyval_t *en = shgetp_null(hm_en, de->key);
        if (en) {
            printf("\"%s\";\"%s\"\n", de->value, en->value);
        }
    }
    hmfree(hm_en);
    hmfree(hm_de);
    return EXIT_SUCCESS;
}