cmake_minimum_required(VERSION 3.10)

project(riscvemu)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCES "src/*.cpp")

add_executable(riscvemu ${SOURCES})

target_include_directories(riscvemu PRIVATE include)