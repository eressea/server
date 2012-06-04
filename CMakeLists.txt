cmake_minimum_required(VERSION 2.4)
project (core C)

add_subdirectory (../dependencies "${CMAKE_CURRENT_BINARY_DIR}/dependencies")
add_subdirectory (src eressea)
