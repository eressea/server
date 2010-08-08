#include <settings.h>
#include <platform.h>
#include "stdafx.h"

#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4127)
#include <sqlite3.c>
#pragma warning(pop)

#include <md5.c>

#include <bson/bson.c>
#include <bson/numbers.c>

#ifndef DISABLE_TESTS
#include <cutest/CuTest.c>
#endif
