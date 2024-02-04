cmake_minimum_required(VERSION 3.27)
project(Mymuduo)

set(CMAKE_CXX_STANDARD 20)

add_executable(Mymuduo
        main.cpp
        Any.hpp
        Buffer.hpp
        Log.hpp
        Socket.hpp
        Channel.hpp
        Poller.hpp
        EventLoop.hpp
        Timer.hpp
        TimeWheel.hpp
        #        server.cpp
#        client.cpp
)
