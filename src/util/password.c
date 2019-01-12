#ifdef _MSC_VER
#include <platform.h>
#endif
#include "password.h"

#include "crypto/crypto.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

int bcrypt_workfactor = 8;

bool password_is_implemented(cryptalgo_t algo) {
    if (algo == PASSWORD_BCRYPT) return true;
    return algo == PASSWORD_PLAINTEXT;
}

const char * password_hash(const char * passwd, cryptalgo_t algo) {
    if (algo == PASSWORD_BCRYPT && bcrypt_workfactor != 0) {
        char salt[BCRYPT_HASHSIZE];
        static char hash[BCRYPT_HASHSIZE];
        int ret;
        bcrypt_gensalt(bcrypt_workfactor, salt);
        ret = bcrypt_hashpw(passwd, salt, hash);
        assert(ret == 0);
        return hash;
    }
    return passwd;
}

int password_verify(const char * pwhash, const char * passwd) {
    if (pwhash[0] == '$') {
        if (pwhash[1] == '2') {
            if (bcrypt_workfactor > 0) {
                int ret = bcrypt_checkpw(passwd, pwhash);
                assert(ret != -1);
                return (ret == 0) ? VERIFY_OK : VERIFY_FAIL;
            }
            return VERIFY_OK;
        }
    }
    return (strcmp(passwd, pwhash) == 0) ? VERIFY_OK : VERIFY_FAIL;
}

void password_generate(char *password, size_t length) {
    char salt[BCRYPT_HASHSIZE];

    assert(BCRYPT_HASHSIZE - 7 > length);
    bcrypt_gensalt(4, salt);
    memcpy(password, salt + 7, length);
    password[length] = 0;
}
