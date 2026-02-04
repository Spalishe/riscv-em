cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(riscvem)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCES "src/*.cpp")
file(GLOB DEVICE_SOURCES "src/devices/*.cpp")
file(GLOB INSTR_SETS "src/sets/*.cpp")

add_executable(riscvem ${SOURCES} ${DEVICE_SOURCES} ${INSTR_SETS})

set(CMAKE_CXX_FLAGS_DEBUG "-latomic -O3 -march=native -g")
set(CMAKE_CXX_FLAGS_RELEASE "-latomic -O3 -march=native")

target_include_directories(riscvem PRIVATE include)
