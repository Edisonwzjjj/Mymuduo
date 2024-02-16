#pragma once

#include "Request.hpp"

constexpr int MAX_LINE = 8192;

enum class HttpRecvState {
    RECV_HTTP_ERROR,
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER
};

class Context {
private:
    int state_code_{200};
    HttpRecvState state_{HttpRecvState::RECV_HTTP_LINE};
    Request request_;

private:
    bool ParseLine(const std::string &line) {
        std::smatch matches;
        std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?", std::regex::icase);
        bool ret = std::regex_match(line, matches, e);
        if (!ret) {
            state_ = HttpRecvState::RECV_HTTP_ERROR;
            state_code_ = 400;
            return false;
        }

        request_.method_ = matches[1];
        std::transform(request_.method_.begin(), request_.method_.end(), request_.method_.begin(), ::toupper);
        request_.path_ = Util::UrlDecode(matches[2]);
        request_.version_ = matches[4];
        auto query = matches[3];
        auto array = Util::Split(query, "&");
        for (auto &str: array) {
            size_t pos = str.find('=');
            if (pos == std::string::npos) {
                state_code_ = 400;
                state_ = HttpRecvState::RECV_HTTP_ERROR;
                return false;
            }
            auto key = Util::UrlDecode(str.substr(0, pos), true);
            auto value = Util::UrlDecode(str.substr(pos), true);
            request_.SetParam(key, value);
        }
        return true;
    }

    bool ParseHead(std::string& line) {
        if (line.back() == '\n') line.pop_back();
        if (line.back() == '\r') line.pop_back();
        size_t pos = line.find(": ");
        if (pos == std::string::npos) {
            state_ = HttpRecvState::RECV_HTTP_ERROR;
            state_code_ = 400;//
            return false;
        }
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 2);
        request_.SetHeader(key, val);
        return true;
    }

    bool RecvHttpLine(Buffer &buf) {
        if (state_ != HttpRecvState::RECV_HTTP_LINE) { return false; }
        auto line = buf.GetLine();
        if (line.empty()) {
            if (buf.ReadableSize() > MAX_LINE) {
                state_code_ = 414;
                state_ = HttpRecvState::RECV_HTTP_ERROR;
                return false;
            }
            return true;
        }
        if (line.size() > MAX_LINE) {
            state_code_ = 414;
            state_ = HttpRecvState::RECV_HTTP_ERROR;
            return false;
        }
        bool res = ParseLine(line);
        if (!res) {
            return false;
        }
        state_ = HttpRecvState::RECV_HTTP_HEAD;
        return true;
    }

    bool RecvHttpHead(Buffer &buf) {
        if (state_ != HttpRecvState::RECV_HTTP_HEAD) { return false; }

        while(true) {
            auto line = buf.GetLine();
            if (line.empty()) {
                if (buf.ReadableSize() > MAX_LINE) {
                    state_ = HttpRecvState::RECV_HTTP_ERROR;
                    state_code_ = 414;
                    return false;
                }
                return true;
            }
            if (line.size() > MAX_LINE) {
                state_ = HttpRecvState::RECV_HTTP_ERROR;
                state_code_ = 414;//URI TOO LONG
                return false;
            }
            if (line == "\n" || line == "\r\n") {
                break;
            }
            bool ret = ParseHead(line);
            if (!ret) {
                return false;
            }
        }
        state_ = HttpRecvState::RECV_HTTP_BODY;
        return true;
    }

    bool RecvHttpBody(Buffer &buf) {
        if (state_ != HttpRecvState::RECV_HTTP_BODY) { return false; }
        size_t sz = request_.ContentLength();
        if (!sz) {
            state_ = HttpRecvState::RECV_HTTP_OVER;
            return true;
        }
        size_t remain = sz - request_.body_.size();
        if (buf.ReadableSize() >= remain) {
            state_ = HttpRecvState::RECV_HTTP_OVER;
        }
        request_.body_ += buf.Read(buf.ReadableSize());
        return true;
    }

public:
    Context() = default;

    Request GetRequest() const {
        return request_;
    }

    int GetStateCode() const {
        return state_code_;
    }

    HttpRecvState GetState() const {
        return state_;
    }

    void Reset() {
        state_code_ = 0;
        state_ = HttpRecvState::RECV_HTTP_LINE;
        request_.Reset();
    }

    void Recv(Buffer &buf) {
        switch (state_) {
            case HttpRecvState::RECV_HTTP_LINE: {
                RecvHttpLine(buf);
            }
            case HttpRecvState::RECV_HTTP_HEAD: {
                RecvHttpHead(buf);
            }
            case HttpRecvState::RECV_HTTP_BODY: {
                RecvHttpBody(buf);
            }
            default:
                break;
        }
    }
};