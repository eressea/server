#pragma once

#include <stdbool.h>
#include <stddef.h>
typedef enum cryptalgo_t {
    PASSWORD_PLAINTEXT,
    PASSWORD_BCRYPT
} cryptalgo_t;
#define PASSWORD_DEFAULT PASSWORD_BCRYPT
#define PASSWORD_MAXSIZE 32

extern int bcrypt_workfactor;

#define VERIFY_OK 0
#define VERIFY_FAIL 1
#define VERIFY_UNKNOWN 2
int password_verify(const char *hash, const char *passwd);
const char * password_hash(const char *passwd, cryptalgo_t algo);
bool password_is_implemented(cryptalgo_t algo);
void password_generate(char *password, size_t length);
