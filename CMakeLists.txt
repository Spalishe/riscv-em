cmake_minimum_required(VERSION 3.10)

# The project name is rather unimportant and you can name it anything you want.
# It just mustn't conflict with the module name that is defined further below.
project(gmod-module-base LANGUAGES CXX)

set(CMAKE_CONFIGURATION_TYPES Release Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Load the gmod-module-base project that sets up headers and settings.
add_subdirectory(./include)
include_directories(./src/include)

file(GLOB SOURCES "src/*.cpp")

# The library name defined here (in this case, "testmodule") will be the name of the module.
add_library(riscv-em SHARED)

# Add the source files required for the module.
# Again, the file names don't matter and you can even add multiple files by adding more space-delimited filenames after "module.cpp".
target_sources(riscv-em PRIVATE ${SOURCES})

target_link_libraries(riscv-em gmod-module-base)
target_link_libraries(riscv-em /home/michen/dev/riscv-em-gmod/slib_x86_64.lib)

target_link_options(riscv-em PRIVATE
    -static-libstdc++
    -static-libgcc
	-static
)

set_gmod_suffix_prefix(riscv-em)
