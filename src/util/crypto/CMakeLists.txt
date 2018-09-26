cmake_minimum_required(VERSION 2.6)
project (crypto C)

IF (MSVC)
  include (MSVC)
  MSVC_SET_WARNING_LEVEL(3)
ENDIF (MSVC)

SET (LIB_SRC
  bcrypt.c
  crypt_blowfish/wrapper.c
  crypt_blowfish/crypt_blowfish.c
  crypt_blowfish/crypt_gensalt.c
)
ADD_LIBRARY (crypto ${LIB_SRC})

set (CRYPTO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "cJSON headers")
set (CRYPTO_LIBRARIES crypto CACHE INTERNAL "crypto libraries")

IF(WIN32)
  FIND_LIBRARY(WIN32_CNG_LIBRARY bcrypt)
  SET(CRYPTO_LIBRARIES ${CRYPTO_LIBRARIES} ${WIN32_CNG_LIBRARY} CACHE
    INTERNAL "crypto libraries")
ENDIF()
            
IF (MSVC)
  MSVC_CRT_SECURE_NO_WARNINGS (crypto)
ENDIF (MSVC)
