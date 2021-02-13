# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindIniParser
-----------

.. versionadded:: 3.20

Find the IniParser libraries, v3

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Devillard::IniParser``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``IniParser_INCLUDE_DIRS``
  where to find sqlite3.h, etc.
``IniParser_LIBRARIES``
  the libraries to link against to use IniParser.
``IniParser_VERSION``
  version of the IniParser library found
``IniParser_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(IniParser_INCLUDE_DIR iniparser.h PATH_SUFFIXES iniparser)
mark_as_advanced(IniParser_INCLUDE_DIR)

# Look for the necessary library
find_library(IniParser_LIBRARY iniparser)
mark_as_advanced(IniParser_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IniParser
    REQUIRED_VARS IniParser_INCLUDE_DIR IniParser_LIBRARY
    VERSION_VAR IniParser_VERSION)

# Create the imported target
if(IniParser_FOUND)
    set(IniParser_INCLUDE_DIRS ${IniParser_INCLUDE_DIR})
    set(IniParser_LIBRARIES ${IniParser_LIBRARY})
    if(NOT TARGET Devillard::IniParser)
        add_library(Devillard::IniParser UNKNOWN IMPORTED)
        set_target_properties(Devillard::IniParser PROPERTIES
            IMPORTED_LOCATION             "${IniParser_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${IniParser_INCLUDE_DIR}")
    endif()
endif()

