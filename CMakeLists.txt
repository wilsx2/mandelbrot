cmake_minimum_required(VERSION 3.18)
project(wacfrac LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(ProcessorCount)
ProcessorCount(N_CORES)
if(N_CORES GREATER 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N_CORES})
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release / RelWithDebInfo optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-O3>
        $<$<COMPILE_LANGUAGE:CXX>:-funroll-loops>
        $<$<COMPILE_LANGUAGE:CXX>:-ffast-math>
        $<$<COMPILE_LANGUAGE:CXX>:-march=native>
        $<$<COMPILE_LANGUAGE:CXX>:-mtune=native>
    )
endif()

# SYCL / CUDA backend configuration
option(ENABLE_CUDA_BACKEND "Build with SYCL CUDA backend" ON)
set(SYCL_CUDA_ARCH "sm_80" CACHE STRING "Target CUDA SM architectures (semicolon seperated)")
set(SYCL_TARGETS "spir64")
if(ENABLE_CUDA_BACKEND)
    list(APPEND SYCL_TARGETS "nvptx64-nvidia-cuda")
endif()
list(JOIN SYCL_TARGETS "," SYCL_TARGETS_STR)

set(SYCL_FLAGS -fsycl "-fsycl-targets=${SYCL_TARGETS_STR}")
if(ENABLE_CUDA_BACKEND)
    foreach(arch ${SYCL_CUDA_ARCH})
        list(APPEND SYCL_FLAGS -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch=${arch})
    endforeach()
    message(STATUS "SYCL CUDA backend enabled, targeting: ${SYCL_CUDA_ARCH}")
else()
    message(STATUS "SYCL CUDA backend disabled, building for ${SYCL_TARGETS_STR} only")
endif()
 
# Packages
find_package(GMP REQUIRED)
find_package(MPFR REQUIRED)
find_package(MPC REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS log log_setup)
 
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG v3.2
)
FetchContent_MakeAvailable(argparse)
 
# Static Library
add_library(libwacfrac STATIC
    src/wacfrac/analysis.cpp
    src/wacfrac/color.cpp
    src/wacfrac/viewport.cpp
    src/wacfrac/io.cpp
    src/wacfrac/log.cpp
    src/wacfrac/renderer.cpp
)
set_target_properties(libwacfrac PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(libwacfrac PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(libwacfrac PUBLIC
    MPC::mpc
    MPFR::mpfr
    GMP::gmp
    Boost::headers
    Boost::log
    Boost::log_setup
)
target_compile_options(libwacfrac PRIVATE ${SYCL_FLAGS})
target_link_options(libwacfrac PRIVATE ${SYCL_FLAGS})

# Application
add_executable(wacfrac src/main.cpp src/cli_options.cpp)
target_link_libraries(wacfrac PRIVATE
    libwacfrac
    argparse
)
target_compile_options(wacfrac PRIVATE ${SYCL_FLAGS})
target_link_options(wacfrac PRIVATE ${SYCL_FLAGS})
