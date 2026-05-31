cmake_minimum_required(VERSION 3.14)
project(riscv CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(riscv_lib src/cpu.cpp)
target_include_directories(riscv_lib PUBLIC src)

add_executable(riscv src/main.cpp)
target_link_libraries(riscv PRIVATE riscv_lib)

enable_testing()
add_subdirectory(tests)
