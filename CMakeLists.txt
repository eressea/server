cmake_minimum_required(VERSION 2.6)
project (server C)

enable_testing()

add_subdirectory (../dependencies "${CMAKE_CURRENT_BINARY_DIR}/dependencies")
add_subdirectory (../core/src "${CMAKE_CURRENT_BINARY_DIR}/core")

add_subdirectory(src server)
