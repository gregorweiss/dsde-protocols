cmake_minimum_required(VERSION 3.20)
project(SDE_proxy)

set(CMAKE_CXX_STANDARD 17)

add_executable(SDE_proxy
        main.cpp
        pex.cpp
        pcx.cpp
        rsx.cpp
        nbx.cpp
        helper.cpp)
