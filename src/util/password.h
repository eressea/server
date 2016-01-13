#pragma once

#define PASSWORD_PLAIN  0
#define PASSWORD_MD5 1
#define PASSWORD_BCRYPT 2 // not implemented
#define PASSWORD_SHA256 5 // not implemented
#define PASSWORD_SHA512 6 // not implemented
#define PASSWORD_DEFAULT PASSWORD_PLAIN


#define VERIFY_OK 0 // password matches hash
#define VERIFY_FAIL 1 // password is wrong
#define VERIFY_UNKNOWN 2 // hashing algorithm not supported
int password_verify(const char * hash, const char * passwd);
const char * password_hash(const char * passwd, int algo);
