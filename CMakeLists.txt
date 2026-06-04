cmake_minimum_required(VERSION 3.15)
project(mandelbrot LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options("$<$<CONFIG:Release>:-O3>")

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET ffmpeg)

add_executable(naive_c "src/naive.c")
target_link_libraries(naive_c PRIVATE m)
add_executable(naive_cpp "src/naive.cpp")
# target_link_libraries(naive PRIVATE PkgConfig::ffmpeg)
