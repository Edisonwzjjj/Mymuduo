cmake_minimum_required(VERSION 3.27)
project(Mymuduo)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        server
        server.cpp
        Buffer.hpp
        Log.hpp
        Socket.hpp
        Channel.hpp
        Poller.hpp
        EventLoop.hpp
        Timer.hpp
        TimeWheel.hpp
        Connection.hpp
        Acceptor.hpp
        Thread.hpp
        ThreadPool.hpp
        TcpServer.hpp
        http/Util.hpp
)

add_executable(
        client
        client.cpp
        Socket.hpp
        http/Util.hpp
)

