project(util C)

SET(_TEST_FILES
base36.test.c
parser.test.c
password.test.c
attrib.test.c
strings.test.c
bsdstring.test.c
functions.test.c
log.test.c
umlaut.test.c
unicode.test.c
rng.test.c
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
goodies.c
language.c
lists.c
log.c
message.c
nrmessage.c
parser.c
password.c
rand.c
resolve.c
strings.c
translation.c
umlaut.c
unicode.c
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
