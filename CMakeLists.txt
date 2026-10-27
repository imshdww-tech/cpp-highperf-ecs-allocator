cmake_minimum_required(VERSION 3.16)
project(HighPerfECSAllocator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(EngineBenchmark src/main.cpp)

if(MSVC)
    target_compile_options(EngineBenchmark PRIVATE /W4 /O2)
else()
    target_compile_options(EngineBenchmark PRIVATE -Wall -Wextra -O3)
endif()
