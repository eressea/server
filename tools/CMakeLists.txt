cmake_minimum_required(VERSION 2.9)
project (tools C)

add_executable(inifile inifile.c)
target_link_libraries(inifile ${IniParser_LIBRARIES})
target_include_directories (inifile PRIVATE ${IniParser_INCLUDE_DIRS})

add_executable(gethash gethash.c)

add_executable(atoi36 atoi36.c ${CMAKE_SOURCE_DIR}/src/util/base36.c)
target_include_directories(atoi36 PRIVATE ${CMAKE_SOURCE_DIR}/src/util)
