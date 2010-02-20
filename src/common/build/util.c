#include "common/settings.h"
#include "common/config.h"
#include "stdafx.h"

#include "common/iniparser/iniparser.c"
#include "common/util/attrib.c"
#include "common/util/argstack.c"
#include "common/util/base36.c"
#include "common/util/crmessage.c"
#include "common/util/cvector.c"
#include "common/util/dice.c"
#include "common/util/event.c"
#include "common/util/eventbus.c"
#include "common/util/filereader.c"
#include "common/util/functions.c"
#include "common/util/goodies.c"
#include "common/util/language.c"
#include "common/util/lists.c"
#include "common/util/log.c"
#include "common/util/message.c"
#include "common/util/mt19937ar.c"
#include "common/util/nrmessage.c"
#include "common/util/parser.c"
#include "common/util/rand.c"
#include "common/util/resolve.c"
#include "common/util/sql.c"
#include "common/util/translation.c"
#include "common/util/umlaut.c"
#include "common/util/unicode.c"
#include "common/util/xml.c"

#ifndef HAVE_INLINE
#include "common/util/bsdstring.c"
#endif

#ifdef __GNUC__
#include "common/util/strncpy.c"
#endif
