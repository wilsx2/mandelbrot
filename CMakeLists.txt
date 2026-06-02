cmake_minimum_required(VERSION 3.15)
project(mandelbrot LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_compile_options(-Wall -Wextra -Werror)

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET ffmpeg)

add_executable(naive "src/naive.c")
# target_link_libraries(naive PRIVATE PkgConfig::ffmpeg)
