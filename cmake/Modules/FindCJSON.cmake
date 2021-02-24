# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindCJSON
-----------

.. versionadded:: 3.20

Find the cJSON libraries, v3

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``DaveGamble::CJSON``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``CJSON_INCLUDE_DIRS``
  where to find sqlite3.h, etc.
``CJSON_LIBRARIES``
  the libraries to link against to use CJSON.
``CJSON_VERSION``
  version of the CJSON library found
``CJSON_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(CJSON_INCLUDE_DIR cJSON.h PATH_SUFFIXES cjson)
mark_as_advanced(CJSON_INCLUDE_DIR)

# Look for the necessary library
find_library(CJSON_LIBRARY cjson)
mark_as_advanced(CJSON_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CJSON
    REQUIRED_VARS CJSON_INCLUDE_DIR CJSON_LIBRARY
    VERSION_VAR CJSON_VERSION)

# Create the imported target
if(CJSON_FOUND)
    set(CJSON_INCLUDE_DIRS ${CJSON_INCLUDE_DIR})
    set(CJSON_LIBRARIES ${CJSON_LIBRARY})
    if(NOT TARGET DaveGamble::CJSON)
        add_library(DaveGamble::CJSON UNKNOWN IMPORTED)
        set_target_properties(DaveGamble::CJSON PROPERTIES
            IMPORTED_LOCATION             "${CJSON_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CJSON_INCLUDE_DIR}")
    endif()
endif()

