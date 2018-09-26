cmake_minimum_required(VERSION 2.6)
project (bcrypt C)

IF (MSVC)
    include (MSVC)
    MSVC_SET_WARNING_LEVEL(3)
ENDIF (MSVC)

SET (LIB_SRC bcrypt.c wrapper.c crypt_blowfish.c crypt_gensalt.c)
ADD_LIBRARY (bcrypt ${LIB_SRC})

set (BCRYPT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "cJSON headers")
set (BCRYPT_LIBRARIES bcrypt CACHE INTERNAL "bcrypt libraries")

IF(UNIX AND NOT APPLE)
    FIND_LIBRARY(UNIX_MATH_LIBRARY m)
    SET(BCRYPT_LIBRARIES ${BCRYPT_LIBRARIES} ${UNIX_MATH_LIBRARY} CACHE
	    INTERNAL "bcrypt libraries")
ENDIF()
            
IF (MSVC)
    MSVC_CRT_SECURE_NO_WARNINGS (bcrypt)
ENDIF (MSVC)

