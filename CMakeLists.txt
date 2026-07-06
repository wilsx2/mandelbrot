cmake_minimum_required(VERSION 3.18)
project(wacfrac LANGUAGES C CXX CUDA)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
add_compile_options($<$<COMPILE_LANGUAGE:CUDA>:-diag-suppress=20014>) # Boost host-only warnings in CUDA compilation

set(CMAKE_CUDA_ARCHITECTURES "86" CACHE STRING "CUDA compute capability")
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)
find_package(CUDAToolkit REQUIRED)

# Generate .clangd config for clangd LSP if using Clang
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  get_filename_component(_clangd_cuda_path ${CMAKE_CUDA_COMPILER} DIRECTORY)
  get_filename_component(_clangd_cuda_path ${_clangd_cuda_path} DIRECTORY)
  set(CLANGD_CUDA_PATH "${_clangd_cuda_path}")
  set(CLANGD_CUDA_ARCH "${CMAKE_CUDA_ARCHITECTURES}")
  set(CLANGD_CXX_STANDARD "${CMAKE_CXX_STANDARD}")
  set(CLANGD_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
  set(CLANGD_VCPKG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
  set(CLANGD_CUDA_INCLUDE_DIR "${_clangd_cuda_path}/targets/x86_64-linux/include")
  set(CLANGD_CUDA_CCCL_INCLUDE_DIR "${CLANGD_CUDA_INCLUDE_DIR}/cccl")
  configure_file(
    "${CMAKE_SOURCE_DIR}/.clangd.in"
    "${CMAKE_SOURCE_DIR}/.clangd"
    @ONLY
  )
endif()

# Static Library
add_library(libwacfrac STATIC
    src/wacfrac/analysis.cpp
    src/wacfrac/color.cpp
    src/wacfrac/viewport.cpp
    src/wacfrac/orbit.cpp
    src/wacfrac/rendering.cpp
    src/wacfrac/io.cpp
    src/wacfrac/bla.cpp
    src/wacfrac/log.cpp
    src/wacfrac/gpu/color.cu
    src/wacfrac/gpu/orbit.cu
    src/wacfrac/gpu/rendering.cu
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

# Benchmarks
add_executable(benchmark src/benchmark.cpp)
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
