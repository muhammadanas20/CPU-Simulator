cmake_minimum_required(VERSION 3.10)
project(CPU_DS_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# All classes are header-only, so each executable has a single .cpp file.
add_executable(cpu_ds src/main.cpp)
add_executable(cpu_ds_tests tests/test_main.cpp)

# GCC 8 needs an extra library for std::filesystem
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  target_link_libraries(cpu_ds stdc++fs)
  target_link_libraries(cpu_ds_tests stdc++fs)
endif()

enable_testing()
add_test(NAME all_tests COMMAND cpu_ds_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
