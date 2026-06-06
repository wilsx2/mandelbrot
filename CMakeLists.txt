cmake_minimum_required(VERSION 3.15)
project(mandelbrot LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options("$<$<CONFIG:Release>:-O3>")

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET ffmpeg)

find_package(Argumentum CONFIG REQUIRED)

add_executable(mplot "src/main.cpp")
target_link_libraries(mplot
   PRIVATE
   Argumentum::argumentum
)
# target_link_libraries(naive PRIVATE PkgConfig::ffmpeg)
