#include <platform.h>
#include "password.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

bool password_is_implemented(int algo) {
    return algo == PASSWORD_PLAINTEXT;
}

const char * password_encode(const char * passwd, int algo) {
    return passwd;
}

int password_verify(const char * pwhash, const char * passwd) {
    return (strcmp(passwd, pwhash) == 0) ? VERIFY_OK : VERIFY_FAIL;
}
