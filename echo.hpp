#pragma once

#include "TcpServer.hpp"

class Echo{
    private:
        TcpServer _server;
    private:
        void OnConnected(const PtrConnection &conn) {
            DBG_LOG("NEW CONNECTION:%p", conn.get());
        }
        void OnClosed(const PtrConnection &conn) {
            DBG_LOG("CLOSE CONNECTION:%p", conn.get());
        }
        void OnMessage(const PtrConnection &conn, Buffer *buf) {
            conn->Send(buf->ReadPosition(), buf->ReadableSize());
            buf->MoveReadOffset(buf->ReadableSize());
            conn->Shutdown();
        }
    public:
        Echo(int port):_server(port) {
            _server.SetThreadCount(std::thread::hardware_concurrency());
            _server.EnableInactiveRelease(10);
            _server.SetClosedCallback(std::bind(&Echo::OnClosed, this, std::placeholders::_1));
            _server.SetConnectedCallback(std::bind(&Echo::OnConnected, this, std::placeholders::_1));
            _server.SetMessageCallback(std::bind(&Echo::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
        }
        void Start() { _server.Start(); }
};