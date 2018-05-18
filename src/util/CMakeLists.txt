project(util C)

SET(_TEST_FILES
attrib.test.c
base36.test.c
# bsdstring.test.c
# crmessage.test.c
# dice.test.c
# event.test.c
# filereader.test.c
functions.test.c
gamedata.test.c
# goodies.test.c
language.test.c
# lists.test.c
# log.test.c
message.test.c
# nrmessage.test.c
parser.test.c
password.test.c
# rand.test.c
# resolve.test.c
rng.test.c
strings.test.c
functions.test.c
log.test.c
# translation.test.c
umlaut.test.c
unicode.test.c
variant.test.c
# xml.test.c
)

SET(_FILES
attrib.c
base36.c
bsdstring.c
crmessage.c
dice.c
event.c
filereader.c
functions.c
gamedata.c
goodies.c
language.c
lists.c
log.c
message.c
mt19937ar.c
nrmessage.c
parser.c
password.c
path.c
pofile.c
rand.c
resolve.c
strings.c
translation.c
umlaut.c
unicode.c
variant.c
xml.c
)

FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(UTIL_SRC ${_SOURCES} PARENT_SCOPE)
FOREACH(_FILE ${_TEST_FILES})
    LIST(APPEND _TESTS ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(UTIL_TESTS ${_TESTS} PARENT_SCOPE)
