cmake_minimum_required(VERSION 3.10)

project(riscvem)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCES "src/*.cpp")
file(GLOB DEVICE_SOURCES "src/devices/*.cpp")

add_executable(riscvem ${SOURCES} ${DEVICE_SOURCES})

set(CMAKE_CXX_FLAGS_DEBUG "-O3 -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

target_include_directories(riscvem PRIVATE include)