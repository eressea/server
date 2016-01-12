#pragma once

#define VERIFY_OK 0 // password matches hash
#define VERIFY_FAIL 1 // password is wrong
#define VERIFY_UNKNOWN 2 // hashing algorithm not supported
int password_verify(const char * hash, const char * passwd);
const char * password_hash(const char * passwd);
