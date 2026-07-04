cmake_minimum_required(VERSION 3.18)
project(wacfrac LANGUAGES C CXX CUDA)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Boost REQUIRED COMPONENTS log log_setup)
find_package(Argumentum CONFIG REQUIRED)
find_package(benchmark REQUIRED)
find_package(TBB REQUIRED)
find_library(GMP_LIB  gmp  REQUIRED HINTS ${CMAKE_PREFIX_PATH}/lib ${CMAKE_FIND_ROOT_PATH}/lib)
find_library(MPFR_LIB mpfr REQUIRED HINTS ${CMAKE_PREFIX_PATH}/lib ${CMAKE_FIND_ROOT_PATH}/lib)
find_library(MPC_LIB  mpc  REQUIRED HINTS ${CMAKE_PREFIX_PATH}/lib ${CMAKE_FIND_ROOT_PATH}/lib)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CUDA_COMPILER /opt/cuda/bin/nvcc)
set(CMAKE_CUDA_ARCHITECTURES "86")
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)
find_package(CUDAToolkit REQUIRED)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wextra>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Werror>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-array-bounds>) # Required to include Boost logging
add_compile_options($<$<COMPILE_LANGUAGE:CUDA>:-diag-suppress=20014>) # Boost host-only warnings in CUDA compilation
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    ${MPC_LIB}
    ${MPFR_LIB}
    ${GMP_LIB}
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
