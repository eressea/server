cmake_minimum_required(VERSION 2.6)
project (arda-server C)

enable_testing()

find_package (Lua 5 REQUIRED)
find_package (ToLua REQUIRED)
find_package (LibXml2 REQUIRED)
find_package (Curses REQUIRED)
find_package (SQLite3 REQUIRED)

add_subdirectory (crypto)
add_subdirectory (quicklist)
add_subdirectory (iniparser)
add_subdirectory (cutest)
add_subdirectory (critbit)
add_subdirectory (eressea/src eressea)
add_subdirectory (src server)
