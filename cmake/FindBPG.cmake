include(FindPackageHandleStandardArgs)
find_path(BPG_INCLUDE_DIR libbpg.h)
find_library(BPG_LIBRARY NAMES bpg)

find_package_handle_standard_args(BPG
 DEFAULT_MSG
  BPG_INCLUDE_DIR
  BPG_LIBRARY
)

mark_as_advanced(BPG_LIBRARY BPG_INCLUDE_DIR)

if(BPG_FOUND)
  set(BPG_LIBRARIES    ${BPG_LIBRARY})
  set(BPG_INCLUDE_DIRS ${BPG_INCLUDE_DIR})
  add_library(BPG::BPG UNKNOWN IMPORTED)
  set_target_properties(BPG::BPG PROPERTIES
        IMPORTED_LOCATION "${BPG_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${BPG_INCLUDE_DIR}")
endif()
