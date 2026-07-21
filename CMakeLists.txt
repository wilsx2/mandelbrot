cmake_minimum_required(VERSION 3.18)
project(wacfrac LANGUAGES C CXX CUDA)

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

if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-linux")
    endif()
endif()
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

find_package(Boost CONFIG REQUIRED COMPONENTS log log_setup)
find_package(Argumentum CONFIG QUIET)
if (NOT Argumentum_FOUND)
    include(FetchContent)
    set(ARGUMENTUM_BUILD_STATIC_LIBS ON)
    FetchContent_Declare(argumentum
        GIT_REPOSITORY https://github.com/mmahnic/argumentum.git
        GIT_TAG v0.3.2
    )
    FetchContent_MakeAvailable(argumentum)
endif()
find_package(benchmark REQUIRED)
find_package(TBB REQUIRED)

find_package(GMP REQUIRED)
find_package(MPFR REQUIRED)
find_package(MPC REQUIRED)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -diag-suppress=20014") # Boost host-only warnings in CUDA compilation
endif()
# Release / RelWithDebInfo optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    # Clang assumed for both C++ and CUDA
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX,CUDA>:-O3>
        $<$<COMPILE_LANGUAGE:CXX,CUDA>:-funroll-loops>
        $<$<COMPILE_LANGUAGE:CXX,CUDA>:-ffast-math>
        $<$<COMPILE_LANGUAGE:CXX>:-march=native>
        $<$<COMPILE_LANGUAGE:CXX>:-mtune=native>
    )
endif()

# CUDA architecture
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES OR CMAKE_CUDA_ARCHITECTURES STREQUAL "")
    set(CMAKE_CUDA_ARCHITECTURES 86) # WARN: Hardcoded, barely portable
endif()
message(STATUS "CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")
set(CMAKE_CUDA_SEPARABLE_COMPILATION OFF)
set(CMAKE_CUDA_SEPARABLE_COMPILATION OFF)
set(CMAKE_CUDA_COMPILE_SEPARABLE_COMPILATION OFF)
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)
find_package(CUDAToolkit REQUIRED)


# Static Library
add_library(libwacfrac STATIC
    src/wacfrac/analysis.cpp
    src/wacfrac/color.cpp
    src/wacfrac/viewport.cpp
    src/wacfrac/io.cpp
    src/wacfrac/log.cpp
    src/wacfrac/renderer_host.cpp
    src/wacfrac/renderer.cu
)
set_target_properties(libwacfrac PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(libwacfrac PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(libwacfrac PUBLIC
    Boost::headers
    Boost::log
    Boost::log_setup
    TBB::tbb
    MPC::mpc
    MPFR::mpfr
    GMP::gmp
    CUDA::cudart
)

add_compile_definitions(
    NOMINMAX
)
target_compile_options(libwacfrac PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:-include "${CMAKE_CURRENT_SOURCE_DIR}/include/wacfrac/cuda_crt_stub.h">
)

# Benchmarks
add_executable(benchmark src/benchmark.cpp)
set_property(TARGET benchmark PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(benchmark PRIVATE
    libwacfrac
    benchmark::benchmark_main
)

# Application
add_executable(wacfrac src/main.cpp)
target_link_libraries(wacfrac PRIVATE
    libwacfrac
    Argumentum::argumentum
)
