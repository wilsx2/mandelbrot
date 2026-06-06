cmake_minimum_required(VERSION 3.18)
project(mandelbrot LANGUAGES C CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Argumentum CONFIG REQUIRED)
find_package(Python 3.14 COMPONENTS Interpreter Development.Module REQUIRED)
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET ffmpeg)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options("$<$<CONFIG:Release>:-O3>")

# Static Library
# 

# Application
add_executable(mplot "src/main.cpp")
target_link_libraries(mplot
   PRIVATE
   Argumentum::argumentum
)

# Python Package (Library and Application)
nanobind_add_module(mplotpy src/bindings.cpp)
