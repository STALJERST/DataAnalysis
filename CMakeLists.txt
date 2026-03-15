cmake_minimum_required(VERSION 4.1)
project(dataAnalis)

set(CMAKE_CXX_STANDARD 20)

add_executable(dataAnalis
        window.cpp
)
target_link_libraries(dataAnalis comctl32)