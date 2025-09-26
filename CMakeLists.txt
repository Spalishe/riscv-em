cmake_minimum_required(VERSION 3.10)

project(riscvem)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCES "src/*.cpp")
file(GLOB DEVICE_SOURCES "src/devices/*.cpp")
file(GLOB JIT_SOURCES "src/jit/*.cpp")

add_executable(riscvem ${SOURCES} ${DEVICE_SOURCES} ${JIT_SOURCES})

find_package(LLVM REQUIRED CONFIG)
target_link_libraries(riscvem PRIVATE LLVM)

set(CMAKE_CXX_FLAGS_DEBUG "-O3 -march=native -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

target_include_directories(riscvem PRIVATE include)
