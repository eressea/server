cmake_minimum_required(VERSION 3.10)
project (tools C)

add_executable(inifile inifile.c)
target_link_libraries(inifile ${INIPARSER_LIBRARY})
target_include_directories (inifile PRIVATE ${INIPARSER_INCLUDE_DIR})

add_executable(gethash gethash.c)

add_executable(atoi36
  atoi36.c
  ${CMAKE_SOURCE_DIR}/src/util/base36.c
  )
target_include_directories(atoi36 PRIVATE ${CMAKE_SOURCE_DIR}/src/util)

install(TARGETS inifile DESTINATION "bin")
