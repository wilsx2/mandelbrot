find_path(MPC_INCLUDE_DIR
    NAMES mpc.h
)

find_library(MPC_LIBRARY
    NAMES mpc libmpc
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPC DEFAULT_MSG MPC_INCLUDE_DIR MPC_LIBRARY)

if(MPC_FOUND)
    if(NOT TARGET MPC::mpc)
        add_library(MPC::mpc UNKNOWN IMPORTED)
        set_target_properties(MPC::mpc PROPERTIES
            IMPORTED_LOCATION "${MPC_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MPC_INCLUDE_DIR}"
        )
    endif()
    mark_as_advanced(MPC_INCLUDE_DIR MPC_LIBRARY)
endif()
