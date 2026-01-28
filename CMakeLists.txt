cmake_minimum_required(VERSION 3.10)
project(il2cpp-dumper)

set(CMAKE_CXX_STANDARD 17)

add_executable(il2cpp-dumper
    src/main.cpp
    src/MetadataLoader.cpp
)

# Add compiler flags for better optimization and warnings
target_compile_options(il2cpp-dumper PRIVATE
    -Wall
    -Wextra
    -O2
)
