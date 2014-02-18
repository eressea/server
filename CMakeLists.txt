cmake_minimum_required(VERSION 2.6)
project (eressea-server C)

enable_testing()

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckSymbolExists)
CHECK_INCLUDE_FILES (stdbool.h HAVE_STDBOOL_H)
CHECK_SYMBOL_EXISTS (_Bool "stdbool.h" HAVE__BOOL)
CHECK_INCLUDE_FILES (strings.h HAVE_STRINGS_H)
CHECK_SYMBOL_EXISTS (strdup "string.h" HAVE_STRDUP)
CHECK_SYMBOL_EXISTS (_strdup "string.h" HAVE__STRDUP)
CHECK_SYMBOL_EXISTS (mkdir "sys/stat.h" HAVE_MKDIR)
CHECK_SYMBOL_EXISTS (_mkdir "direct.h" HAVE__MKDIR)

CONFIGURE_FILE (
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_BINARY_DIR}/include/config.h)
INCLUDE_DIRECTORIES (${CMAKE_BINARY_DIR}/include)

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DHAVE__BOOL")
elseif(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrt.lib")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
ELSE(CMAKE_COMPILER_IS_GNUCC)
    MESSAGE(STATUS "Unknown compiler ${CMAKE_C_COMPILER_ID}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

find_package (LibXml2 REQUIRED)
find_package (SQLite3 REQUIRED)
find_package (Curses REQUIRED)
find_package (Lua REQUIRED)
find_package (ToLua REQUIRED)

add_subdirectory (cutest)
add_subdirectory (crypto)
add_subdirectory (storage)
add_subdirectory (iniparser)
add_subdirectory (quicklist)
add_subdirectory (critbit)
add_subdirectory (src eressea)
