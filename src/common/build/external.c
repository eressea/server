#include "common/settings.h"
#include <platform.h>
#include "stdafx.h"

#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4127)
#include <external/sqlite3.c>
#pragma warning(pop)
#include <external/md5.c>
#include <external/bson/bson.c>
#include <external/bson/numbers.c>
