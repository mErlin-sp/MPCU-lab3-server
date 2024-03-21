cmake_minimum_required(VERSION 3.27)
project(lab3-server)

set(CMAKE_CXX_STANDARD 17)

add_executable(lab3-server main.cpp
        MyClient.cpp
        MyClient.h)
