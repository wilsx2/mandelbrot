cmake_minimum_required(VERSION 3.18)
project(wacfrac LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Boost REQUIRED COMPONENTS log log_setup)
find_package(Argumentum CONFIG REQUIRED)
find_package(benchmark REQUIRED)
find_package(TBB REQUIRED)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-Wno-error=array-bounds)
add_compile_options("$<$<CONFIG:Release>:-O3>")

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
    mpc
    mpfr
    gmp
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
