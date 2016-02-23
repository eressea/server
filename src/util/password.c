#include <platform.h>
#include "password.h"

#include <md5.h>
#include <crypt_blowfish.h>
#include <mtrand.h>
#include <drepper.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAXSALTLEN 32 // maximum length in characters of any salt
#define SALTLEN 8 // length of salts we generate

#define b64_from_24bit(B2, B1, B0, N)					      \
  do {									      \
    unsigned int w = ((B2) << 16) | ((B1) << 8) | (B0);			      \
    int n = (N);							      \
    while (n-- > 0 && buflen > 0)					      \
      {									      \
	*cp++ = itoa64[w & 0x3f];	   		      \
	--buflen;							      \
	w >>= 6;							      \
      }									      \
  } while (0)


char *password_gensalt(char *salt, size_t salt_len) {
    size_t buflen = salt_len-1;
    char *cp = salt;
    while (buflen) {
        unsigned long ul = genrand_int32() & (unsigned long)time(0);
        b64_from_24bit((char)(ul & 0xFF), (char)((ul >> 8) & 0xff), (char)((ul >> 16) & 0xFF), 4);
    }
    salt[salt_len-1] = 0;
    return salt;
}

static bool password_is_implemented(int algo) {
    return algo == PASSWORD_PLAINTEXT || algo == PASSWORD_BCRYPT || algo == PASSWORD_NOCRYPT || algo == PASSWORD_MD5 || algo == PASSWORD_APACHE_MD5;
}

static const char * password_hash_i(const char * passwd, const char *input, int algo, char *result, size_t len) {
    if (algo == PASSWORD_BCRYPT) {
        char salt[MAXSALTLEN];
        char setting[40];
        if (!input) {
            input = password_gensalt(salt, MAXSALTLEN);
        }
        if (_crypt_gensalt_blowfish_rn("$2y$", 5, input, strlen(input), setting, sizeof(setting)) == NULL) {
            return NULL;
        }
        if (_crypt_blowfish_rn(passwd, setting, result, len) == NULL) {
            return NULL;
        }
        return result;
    }
    else if (algo == PASSWORD_PLAINTEXT) {
        _snprintf(result, len, "%s", passwd);
        return result;
    }
    else if (algo == PASSWORD_NOCRYPT) {
        _snprintf(result, len, "$0$%s", passwd);
        return result;
    }
    else if (password_is_implemented(algo)) {
        char salt[MAXSALTLEN];
        assert(passwd);
        if (input) {
            const char * dol = strchr(input, '$');
            size_t salt_len;
            if (dol) {
                assert(dol > input && dol[0] == '$');
                salt_len = dol - input;
            }
            else {
                salt_len = strlen(input);
            }
            assert(salt_len < MAXSALTLEN);
            memcpy(salt, input, salt_len);
            salt[salt_len] = 0;
        } else {
            input = password_gensalt(salt, sizeof(salt));
        }
        if (algo == PASSWORD_MD5) {
            return md5_crypt_r(passwd, input, result, len);
        }
        else if (algo == PASSWORD_APACHE_MD5) {
            apr_md5_encode(passwd, input, result, len);
            return result;
        }
    }
    return NULL;
}

const char * password_encode(const char * passwd, int algo) {
    static char result[64]; // TODO: static result buffers are bad mojo!
    if (algo < 0) algo = PASSWORD_DEFAULT;
    return password_hash_i(passwd, 0, algo, result, sizeof(result));
}

int password_verify(const char * pwhash, const char * passwd) {
    char hash[64];
    int algo = PASSWORD_PLAINTEXT;
    char *pos;
    const char *result;
    assert(passwd);
    assert(pwhash);
    if (pwhash[0] == '$') {
        algo = pwhash[1];
    }
    if (!password_is_implemented(algo)) {
        return VERIFY_UNKNOWN;
    }
    if (algo == PASSWORD_PLAINTEXT) {
        return (strcmp(passwd, pwhash) == 0) ? VERIFY_OK : VERIFY_FAIL;
    } else if (algo == PASSWORD_BCRYPT) {
        char sample[200];
        _crypt_blowfish_rn(passwd, pwhash, sample, sizeof(sample));
        return (strcmp(sample, pwhash) == 0) ? VERIFY_OK : VERIFY_FAIL;
    }
    pos = strchr(pwhash+2, '$');
    assert(pos && pos[0] == '$');
    pos = strchr(pos, '$')+1;
    result = password_hash_i(passwd, pos, algo, hash, sizeof(hash));
    if (strcmp(pwhash, result) == 0) {
        return VERIFY_OK;
    }
    return VERIFY_FAIL;
}
