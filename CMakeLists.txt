cmake_minimum_required(VERSION 2.6)
project (e4-server C)

enable_testing()

add_subdirectory (crypto)
add_subdirectory (quicklist)
add_subdirectory (iniparser)
add_subdirectory (cutest)
add_subdirectory (critbit)
add_subdirectory (eressea/src eressea)
add_subdirectory (src server)
