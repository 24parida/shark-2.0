#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "phevaluator::pheval" for configuration "Release"
set_property(TARGET phevaluator::pheval APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(phevaluator::pheval PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libpheval.a"
  )

list(APPEND _cmake_import_check_targets phevaluator::pheval )
list(APPEND _cmake_import_check_files_for_phevaluator::pheval "${_IMPORT_PREFIX}/lib/libpheval.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
