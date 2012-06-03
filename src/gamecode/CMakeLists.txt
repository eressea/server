cmake_minimum_required (VERSION 2.4)
project (gamecode)

include_directories (..)
include_directories (${LIBXML2_INCLUDE_DIR})
#include_directories (${CRITBIT_INCLUDE_DIR})
include_directories (${QUICKLIST_INCLUDE_DIR})
include_directories (${CUTEST_INCLUDE_DIR})
#include_directories (${INIPARSER_INCLUDE_DIR})
#include_directories (${CRYPTO_INCLUDE_DIR})

add_library (gamecode
#  archetype.c
  creation.c
  creport.c
  economy.c
  economy_test.c
  give.c
  items.c
  laws.c
  laws_test.c
  market.c
  market_test.c
  monster.c
  randenc.c
  report.c
  spy.c
  study.c
  summary.c
  xmlreport.c
)
