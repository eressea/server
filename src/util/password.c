#include <platform.h>
#include "password.h"

#include <md5.h>
#include <mtrand.h>

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
	*cp++ = itoa64[w & 0x3f];					      \
	--buflen;							      \
	w >>= 6;							      \
      }									      \
  } while (0)


char *password_gensalt(void) {
    static char salt[SALTLEN + 1];
    char *cp = salt;
    int buflen = SALTLEN;
    while (buflen) {
        unsigned long ul = genrand_int32() & (unsigned long)time(0);
        b64_from_24bit((char)(ul & 0xFF), (char)((ul>>8)&0xff), (char)((ul>>16)&0xFF), 4);
    }
    salt[SALTLEN] = 0;
    return salt;
}

static const char * password_hash_i(const char * passwd, const char *salt, int algo, char *result, size_t len) {
    assert(passwd);
    if (!salt) {
        salt = password_gensalt();
    }
    if (algo==PASSWORD_PLAIN) {
        _snprintf(result, len, "$0$%s$%s", salt, passwd);
    }
    else if (algo == PASSWORD_MD5) {
        return md5_crypt_r(passwd, salt, result, len);
    }
    else if (algo == PASSWORD_APACHE_MD5) {
        apr_md5_encode(passwd, salt, result, len);
        return result;
    }
    else {
        return NULL;
    }
    return result;
}

const char * password_hash(const char * passwd, const char * salt, int algo) {
    static char result[64]; // TODO: static result buffers are bad mojo!
    if (algo < 0) algo = PASSWORD_DEFAULT;
    return password_hash_i(passwd, salt, algo, result, sizeof(result));
}

static bool password_is_implemented(int algo) {
    return algo==PASSWORD_PLAIN || algo==PASSWORD_MD5 || algo==PASSWORD_APACHE_MD5;
}

int password_verify(const char * pwhash, const char * passwd) {
    char salt[MAXSALTLEN+1];
    char hash[64];
    size_t len;
    int algo;
    char *pos;
    const char *dol, *result;
    assert(passwd);
    assert(pwhash);
    assert(pwhash[0] == '$');
    algo = pwhash[1];
    pos = strchr(pwhash+2, '$');
    assert(pos && pos[0] == '$');
    ++pos;
    dol = strchr(pos, '$');
    assert(dol>pos && dol[0] == '$');
    len = dol - pos;
    assert(len <= MAXSALTLEN);
    strncpy(salt, pos, len);
    salt[len] = 0;
    result = password_hash_i(passwd, salt, algo, hash, sizeof(hash));
    if (!password_is_implemented(algo)) {
        return VERIFY_UNKNOWN;
    }
    if (strcmp(pwhash, result) == 0) {
        return VERIFY_OK;
    }
    return VERIFY_FAIL;
}
