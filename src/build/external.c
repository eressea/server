#include <settings.h>
#include <platform.h>
#include "stdafx.h"

#pragma warning(push)
#pragma warning(disable: 4706) /* '__STDC_VERSION__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */
#pragma warning(disable: 4244) /* '-=' : conversion from 'int' to 'u16', possible loss of data */
#pragma warning(disable: 4127) /* conditional expression is constant */
#pragma warning(disable: 4820) /* 'sqlite3_index_constraint' : '2' bytes padding added after data member 'usable' */
#pragma warning(disable: 4668) /* <name> is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */
#pragma warning(disable: 4242) /* '=' : conversion from 'int' to 'ynVar', possible loss of data */
#include <sqlite3.c>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4668) /* <name> is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */
#include <bson/bson.c>
#pragma warning(pop)

#include <bson/numbers.c>

#include <md5.c>

#ifndef DISABLE_TESTS
#include <cutest/CuTest.c>
#endif
