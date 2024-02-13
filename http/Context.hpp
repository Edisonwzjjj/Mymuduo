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
    int state_code_;
    HttpRecvState state_;
    Request request_;

private:
    bool ParseLine(std::string &line) {
        std::smatch matches;
        std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?", std::regex::icase);
        bool ret = std::regex_match(line, matches, e);
        if (!ret) {
            state_ = HttpRecvState::RECV_HTTP_ERROR;
            state_code_ = 400;
            return false;
        }


    }
};