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

find_package(SDL3 REQUIRED CONFIG)
target_link_libraries(riscvem PRIVATE SDL3)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
target_link_libraries(riscvem PRIVATE OpenGL)

set(CMAKE_CXX_FLAGS_DEBUG "-latomic -O4 -march=native -g")
set(CMAKE_CXX_FLAGS_RELEASE "-latomic -O3 -march=native")

target_include_directories(riscvem PRIVATE include)
