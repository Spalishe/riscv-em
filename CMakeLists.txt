cmake_minimum_required(VERSION 3.10)

project(riscvemu)


# Specify C++ standard (optional)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED True)


# Add executable from all .cpp files in src/

file(GLOB SOURCES "src/*.cpp")


add_executable(riscvemu ${SOURCES})


# Add include directory

target_include_directories(riscvemu PRIVATE include)