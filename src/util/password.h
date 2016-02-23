#pragma once

#define PASSWORD_PLAINTEXT 0
#define PASSWORD_NOCRYPT '0'
#define PASSWORD_MD5 '1'
#define PASSWORD_BCRYPT '2' // not implemented
#define PASSWORD_APACHE_MD5 'a'
#define PASSWORD_SHA256 '5' // not implemented
#define PASSWORD_SHA512 '6' // not implemented
#define PASSWORD_DEFAULT PASSWORD_PLAINTEXT

#define VERIFY_OK 0 // password matches hash
#define VERIFY_FAIL 1 // password is wrong
#define VERIFY_UNKNOWN 2 // hashing algorithm not supported
int password_verify(const char *hash, const char *passwd);
const char * password_encode(const char *passwd, int algo);
