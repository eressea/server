cmake_minimum_required(VERSION 2.9)
project (crypto C)

if (MSVC)
  #add_compile_definitions (_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/WX)
  if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W3" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else()
    add_compile_options(/W3)
  endif()
endif (MSVC)

set (LIB_SRC
  crypto.c
  crypt_blowfish/wrapper.c
  crypt_blowfish/crypt_blowfish.c
  crypt_blowfish/crypt_gensalt.c
)
add_library (crypto ${LIB_SRC})

set (CRYPTO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "crypto headers")
set (CRYPTO_LIBRARIES crypto CACHE INTERNAL "crypto libraries")

if(WIN32)
  set(CRYPTO_LIBRARIES ${CRYPTO_LIBRARIES} bcrypt CACHE
    INTERNAL "crypto libraries")
endif()
