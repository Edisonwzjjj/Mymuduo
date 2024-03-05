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
        explicit Echo(int port):_server(port) {
            _server.SetThreadCount(std::thread::hardware_concurrency());
            _server.EnableInactiveRelease(10);
            _server.SetClosedCallback([this](auto && PH1) { OnClosed(std::forward<decltype(PH1)>(PH1)); });
            _server.SetConnectedCallback([this](auto && PH1) { OnConnected(std::forward<decltype(PH1)>(PH1)); });
            _server.SetMessageCallback([this](auto && PH1, auto && PH2) { OnMessage(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
        }
        void Start() { _server.Start(); }
};