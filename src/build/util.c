#include <settings.h>
#include <platform.h>
#include "stdafx.h"

#include <iniparser/iniparser.c>
#include <mt19937ar.c>

#include <util/console.c>
#include <util/attrib.c>
#include <util/argstack.c>
#include <util/base36.c>
#include <util/crmessage.c>
#include <util/cvector.c>
#include <util/dice.c>
#include <util/event.c>
#include <util/eventbus.c>
#include <util/filereader.c>
#include <util/functions.c>
#include <util/goodies.c>
#include <util/language.c>
#include <util/lists.c>
#include <util/quicklist.c>
#include <util/log.c>
#include <util/message.c>
#include <util/nrmessage.c>
#include <util/parser.c>
#include <util/rand.c>
#include <util/resolve.c>
#include <util/sql.c>
#include <util/translation.c>
#include <util/umlaut.c>
#include <util/unicode.c>
#include <util/xml.c>

#ifndef HAVE_INLINE
#include <util/bsdstring.c>
#endif

#ifdef __GNUC__
#include <util/strncpy.c>
#endif
