#pragma once

#include <utility>

#include "Util.hpp"
#include "Request.hpp"
#include "Responce.hpp"
#include "Context.hpp"

class HttpServer {
private:
    TcpServer server_;
    std::string base_dir_;

    using Handler = std::function<void(const Request &, Response &)>;
    using Handlers = std::vector<std::pair<std::regex, Handler>>;
    Handlers get_route_;
    Handlers post_route_;
    Handlers put_route_;
    Handlers delete_route_;

private:
    static void ErrorHandle(const Request &src, Response &dst) {
        std::string body;
        body += "<html>";
        body += "<head>";
        body += "<meta http-equiv='Content-Type' content='text/html;charset=utf-8'>";
        body += "</head>";
        body += "<body>";
        body += "<h1>";
        body += std::to_string(dst.state_code_);
        body += " ";
        body += Util::StateDesc(dst.state_code_);
        body += "</h1>";
        body += "</body>";
        body += "</html>";
        dst.SetContent(body, "text/html");
    }

    void WriteResponse(const PtrConnection &conn, const Request &request, Response &response) {
        if (request.Close()) {
            response.SetHeader("Connection", "close");
        } else {
            response.SetHeader("Connection", "keep-alive");
        }
        if (!response.body_.empty() && !response.HasHeader("Content-Length")) {
            response.SetHeader("Content-Length", std::to_string(response.body_.size()));
        }
        if (!response.body_.empty() && !response.HasHeader("Content-Type")) {
            response.SetHeader("Content-Type", "application/octet-stream");
        }
        if (response.redirect_) {
            response.SetHeader("Location", response.redirect_url_);
        }

        std::stringstream rsp_str;
        rsp_str << request.version_ << " " << std::to_string(response.state_code_)
                << " " << Util::StateDesc(response.state_code_) << "\r\n";
        for (auto &x: request.headers_) {
            rsp_str << x.first << " " << x.second << "\r\n";
        }
        rsp_str << "\r\n" << response.body_;
        conn->Send(rsp_str.str());
    }

    bool IsFileHandler(const Request &request) const {
        if (base_dir_.empty()) { return false;}
        if (request.method_ != "HEAD" && request.method_ != "GET") { return false; }
        if (!Util::ValidPath(request.path_)) { return false; }
        std::string tmp = base_dir_ + request.path_;
        if (tmp.back() == '/') {
            tmp += "index.html";
        }
        if (!Util::IsRegular(tmp)) { return false; }
        return true;
    }

    void FileHandler(const Request &request, Response &response) {
        std::string tmp = base_dir_ + request.path_;
        if (tmp.back() == '/') {
            tmp += "index.html";
        }
        bool res = Util::ReadFile(tmp, response.body_);
        if (!res) { return; }
        std::string mime = Util::ExtMime(tmp);
        response.SetContent("content-type", mime);
    }

    void Dispatch(Request &request, Response &response, Handlers handlers) {
        for (auto &[k, v]: handlers) {
            bool res = std::regex_match(request.path_, request.match_, k);
            if (!res) continue;
            v(request, response);
            return;
        }
        response.state_code_ = 404;
    }

    void Route(Request &request, Response &response) {
        if (IsFileHandler(request)) {
            FileHandler(request, response);
            return;
        }
        if (request.method_ == "HEAD" || request.method_ == "GET") {
            return Dispatch(request, response, get_route_);
        } else if (request.method_ == "POST") {
            return Dispatch(request, response, post_route_);
        } else if (request.method_ == "DELETE") {
            return Dispatch(request, response, delete_route_);
        } else if (request.method_ == "PUT"){
            return Dispatch(request, response, put_route_);
        }
        response.state_code_ = 405; // method not allowed
    }

    void Conn(const PtrConnection &conn) {
        conn->SetContext(Context());
        DBG_LOG("NEW CONNECTION %p", conn.get());
    }

    void OnMessage(const PtrConnection &conn, Buffer &buf) {
        while (buf.ReadableSize() > 0) {
            auto context = any_cast<Context>(conn->GetContext());
            context->Recv(buf);
            auto request = context->GetRequest();
            Response response(context->GetStateCode());
            if (context->GetStateCode() >= 400) {
                ErrorHandle(request, response);
                WriteResponse(conn, request, response);
                context->Reset();
                buf.Read(buf.ReadableSize());
                conn->Shutdown();
                return;
            }
            if (context->GetState() != HttpRecvState::RECV_HTTP_OVER) {
                return;
            }
            Route(request, response);
            WriteResponse(conn, request, response);
            context->Reset();
            if (request.Close()) {
                conn->Shutdown();
            }
        }
    }

    void SetBase(const std::string &str) {
        assert(Util::IsDir(str));
        base_dir_ = str;
    }

public:
    HttpServer(int port, int timeout): server_(port) {
        server_.EnableRelease(timeout);
        server_.SetConnectedCallback([this](auto && conn) { Conn(conn); });
        server_.SetMessageCallback([this](auto && conn, Buffer & buf) { OnMessage(conn, buf); });
    }

    void Get(const std::string &pattern, const Handler &handler) {
        get_route_.emplace_back(std::regex(pattern), handler);
    }
    void Post(const std::string &pattern, const Handler &handler) {
        post_route_.emplace_back(std::regex(pattern), handler);
    }
    void Put(const std::string &pattern, const Handler &handler) {
        put_route_.emplace_back(std::regex(pattern), handler);
    }
    void Delete(const std::string &pattern, const Handler &handler) {
        delete_route_.emplace_back(std::regex(pattern), handler);
    }

    void Listen() {
        server_.Start();
    }
};