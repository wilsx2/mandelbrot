find_path(MPFR_INCLUDE_DIR
    NAMES mpfr.h
)

find_library(MPFR_LIBRARY
    NAMES mpfr libmpfr
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPFR DEFAULT_MSG MPFR_INCLUDE_DIR MPFR_LIBRARY)

if(MPFR_FOUND)
    if(NOT TARGET MPFR::mpfr)
        add_library(MPFR::mpfr UNKNOWN IMPORTED)
        set_target_properties(MPFR::mpfr PROPERTIES
            IMPORTED_LOCATION "${MPFR_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MPFR_INCLUDE_DIR}"
        )
    endif()
    mark_as_advanced(MPFR_INCLUDE_DIR MPFR_LIBRARY)
endif()
