#ifdef _MSC_VER
#include <platform.h>
#endif
#include "password.h"

#include <bcrypt.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>

bool password_is_implemented(cryptalgo_t algo) {
    if (algo == PASSWORD_BCRYPT) return true;
    return algo == PASSWORD_PLAINTEXT;
}

const char * password_encode(const char * passwd, cryptalgo_t algo) {
    if (algo == PASSWORD_BCRYPT) {
        char salt[BCRYPT_HASHSIZE];
        static char hash[BCRYPT_HASHSIZE];
        int ret;
        bcrypt_gensalt(12, salt);
        ret = bcrypt_hashpw(passwd, salt, hash);
        assert(ret == 0);
        return hash;
    }
    return passwd;
}

int password_verify(const char * pwhash, const char * passwd) {
    if (pwhash[0] == '$') {
        if (pwhash[1] == '2') {
            int ret = bcrypt_checkpw(passwd, pwhash);
            assert(ret != -1);
            return (ret == 0) ? VERIFY_OK : VERIFY_FAIL;
        }
    }
    return (strcmp(passwd, pwhash) == 0) ? VERIFY_OK : VERIFY_FAIL;
}
