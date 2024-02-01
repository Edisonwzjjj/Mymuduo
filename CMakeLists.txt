cmake_minimum_required(VERSION 3.27)
project(Mymuduo)

set(CMAKE_CXX_STANDARD 23)

add_executable(Mymuduo main.cpp
        TimeWheel.cpp
        TimeWheel.h
        Timer.cpp
        Timer.h)
