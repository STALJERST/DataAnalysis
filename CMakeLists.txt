cmake_minimum_required(VERSION 4.1)
project(dataAnalysis)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)

find_package(Boost REQUIRED)

add_executable(dataAnalysis
        window.cpp
        RenderWindow.cpp
        RenderWindow.h
        MemoryEngine.h
        MemoryEngine.cpp
        proxy/ProxyServer.h
)

target_link_libraries(dataAnalysis
        PRIVATE
        wsock32
        comctl32
        Boost::boost
        ws2_32
)


