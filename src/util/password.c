#include <platform.h>
#include "password.h"

#include <md5.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>

#define MAXSALTLEN 32 // maximum length in characters of any salt

static const char * password_hash_i(const char * passwd, const char *salt, int algo, char *result, size_t len) {
    assert(passwd);
    assert(salt);
    if (algo==PASSWORD_PLAIN) {
        _snprintf(result, len, "$0$%s$%s", salt, passwd);
    }
    else if (algo == PASSWORD_MD5) {
        md5_state_t ms;
        md5_byte_t digest[16];
        md5_init(&ms);
        md5_append(&ms, (const md5_byte_t *)passwd, (int)strlen(passwd));
        md5_append(&ms, (const md5_byte_t *)salt, (int)strlen(salt));
        md5_finish(&ms, digest);
        _snprintf(result, len, "$1$%s$%s", salt, digest); // FIXME: need to build a hex string first!
    } 
    else {
        return NULL;
    }
    return result;
}

const char * password_hash(const char * passwd, int algo) {
    static char result[64]; // TODO: static result buffers are bad mojo!
    if (algo < 0) algo = PASSWORD_DEFAULT;
    return password_hash_i(passwd, "saltyfish", PASSWORD_DEFAULT, result, sizeof(result));
}

static bool password_is_implemented(int algo) {
    return algo==PASSWORD_PLAIN || algo==PASSWORD_MD5;
}

int password_verify(const char * pwhash, const char * passwd) {
    char salt[MAXSALTLEN+1];
    char hash[64];
    size_t len;
    int algo;
    char *pos;
    const char *dol;
    assert(pwhash);
    assert(passwd);
    assert(pwhash[0] == '$');
    algo = pwhash[1] - '0';
    pos = strchr(pwhash+2, '$');
    assert(pos[0] == '$');
    ++pos;
    dol = strchr(pos, '$');
    assert(dol>pos && dol[0] == '$');
    len = dol - pos;
    assert(len <= MAXSALTLEN);
    strncpy(salt, pos, len);
    salt[len] = 0;
    password_hash_i(passwd, salt, algo, hash, sizeof(hash));
    if (!password_is_implemented(algo)) {
        return VERIFY_UNKNOWN;
    }
    if (strcmp(pwhash, hash) == 0) {
        return VERIFY_OK;
    }
    return VERIFY_FAIL;
}
