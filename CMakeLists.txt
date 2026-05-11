cmake_minimum_required(VERSION 4.1)
project(dataAnalysis)

set(CMAKE_CXX_STANDARD 20)

add_executable(dataAnalysis
        window.cpp
        RenderWindow.cpp
        RenderWindow.h
)

target_link_libraries(dataAnalysis comctl32)


