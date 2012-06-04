cmake_minimum_required(VERSION 2.4)
project (server)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/Modules/")

add_subdirectory (../dependencies "${CMAKE_CURRENT_BINARY_DIR}/dependencies")
add_subdirectory (../core/src "${CMAKE_CURRENT_BINARY_DIR}/core")

add_subdirectory(src server)
