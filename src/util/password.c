#include <platform.h>
#include "password.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#define PASSWORD_PLAIN  0
#define PASSWORD_MD5 1
#define PASSWORD_SHA256 2
#define PASSWORD_BCRYPT 3 // not implemented
#define PASSWORD_DEFAULT PASSWORD_PLAIN

static const char * password_hash_i(const char * passwd, const char *salt, int algo) {
    static char result[64]; // TODO: static result buffers are bad mojo!
    assert(passwd);
    assert(salt);
    if (algo==PASSWORD_PLAIN) {
        _snprintf(result, sizeof(result), "$0$%s$%s", salt, passwd);
    }
    else {
        return NULL;
    }
    return result;
}

const char * password_hash(const char * passwd) {
    return password_hash_i(passwd, "saltyfish", PASSWORD_DEFAULT);
}

int password_verify(const char * hash, const char * passwd) {
    assert(hash);
    assert(passwd);
    return VERIFY_UNKNOWN;
}
