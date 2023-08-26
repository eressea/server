project(util C)

add_subdirectory (crypto)

SET(_TEST_FILES
# aliases.test.c
base36.test.c
# crmessage.test.c
# dice.test.c
functions.test.c
# goodies.test.c
keyword.test.c
language.test.c
# lists.test.c
# log.test.c
# stats.test.c
message.test.c
# nrmessage.test.c
order_parser.test.c
# param.test.c
parser.test.c
password.test.c
rand.test.c
# rng.test.c
# resolve.test.c
log.test.c
# translation.test.c
umlaut.test.c
unicode.test.c
variant.test.c
)

SET(_FILES
aliases.c
base36.c
crmessage.c
dice.c
functions.c
goodies.c
keyword.c
language.c
lists.c
log.c
stats.c
message.c
mt19937ar.c
nrmessage.c
order_parser.c
param.c
parser.c
password.c
path.c
pofile.c
rand.c
resolve.c
stb.c
translation.c
umlaut.c
unicode.c
variant.c
)

FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(UTIL_SRC ${_SOURCES} PARENT_SCOPE)
FOREACH(_FILE ${_TEST_FILES})
    LIST(APPEND _TESTS ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(UTIL_TESTS ${_TESTS} PARENT_SCOPE)
