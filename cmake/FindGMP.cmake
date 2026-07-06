find_path(GMP_INCLUDE_DIR
    NAMES gmp.h
)

find_library(GMP_LIBRARY
    NAMES gmp libgmp
)

find_library(GMPXX_LIBRARY
    NAMES gmpxx libgmpxx
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP DEFAULT_MSG GMP_INCLUDE_DIR GMP_LIBRARY)

if(GMP_FOUND)
    if(NOT TARGET GMP::gmp)
        add_library(GMP::gmp UNKNOWN IMPORTED)
        set_target_properties(GMP::gmp PROPERTIES
            IMPORTED_LOCATION "${GMP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
        )
    endif()
    if(GMPXX_LIBRARY AND NOT TARGET GMP::gmpxx)
        add_library(GMP::gmpxx UNKNOWN IMPORTED)
        set_target_properties(GMP::gmpxx PROPERTIES
            IMPORTED_LOCATION "${GMPXX_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
        )
    endif()
    mark_as_advanced(GMP_INCLUDE_DIR GMP_LIBRARY GMPXX_LIBRARY)
endif()
