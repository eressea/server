#pragma once

#define PASSWORD_PLAINTEXT 0
#define PASSWORD_DEFAULT PASSWORD_PLAINTEXT

#define VERIFY_OK 0 // password matches hash
#define VERIFY_FAIL 1 // password is wrong
#define VERIFY_UNKNOWN 2 // hashing algorithm not supported
int password_verify(const char *hash, const char *passwd);
const char * password_encode(const char *passwd, int algo);
bool password_is_implemented(int algo);
