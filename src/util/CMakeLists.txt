cmake_minimum_required (VERSION 2.4)
project (util)

find_package (Lua51 REQUIRED)
find_package (LibXml2 REQUIRED)
#find_package (CuTest REQUIRED HINTS ${DEPENDENCIES_DIR})
#find_package (QuickList REQUIRED)
#find_package (CritBit REQUIRED)

include_directories (${LUA_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})
include_directories (..
  ${CRITBIT_INCLUDE_DIR}
  ${QUICKLIST_INCLUDE_DIR}
  ${CUTEST_INCLUDE_DIR}
  )

add_library (util attrib.c
  base36.c
  base36_test.c
  bsdstring.c
  bsdstring_test.c
  console.c
  crmessage.c
  dice.c
  eventbus.c
  event.c
  filereader.c
  functions.c
  functions_test.c
  goodies.c
  language.c
  listbox.c
  lists.c
  log.c
  message.c
  nrmessage.c
  os.c
  parser.c
  rand.c
  resolve.c
  sql.c
  strings.c
  translation.c
  umlaut.c
  umlaut_test.c
  unicode.c
  xml.c
)
