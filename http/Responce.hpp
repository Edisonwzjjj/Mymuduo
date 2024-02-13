#pragma once

#include "Util.hpp"

class Response {
public:
    int state_code_{200};
    bool redirect_{false};

    std::string_view body_;
    std::string_view redirect_url_;
    std::unordered_map<std::string_view, std::string_view> headers_;

public:
    Response() = default;
    explicit Response(int state_code): state_code_(state_code) {}

    void Reset() {

    }

    void SetHeader(std::string_view &key, std::string_view &val) {
        headers_.insert(std::make_pair(key, val));
    }

    bool HasHeader(std::string_view &key) const {
        auto it = headers_.find(key);
        return it != headers_.end();
    }

    std::string_view GetHeader(std::string_view &key) {
        bool st  = HasHeader(key);
        if (st) {
            return headers_[key];
        }
        return "";
    }

    void SetContent(std::string_view &body,  std::string_view &type) {
        body_ = body;
        std::string_view key("Content-Type");
        SetHeader(key, type);
    }

    void SetRedirect(std::string_view &url, int state_code = 302) {
        state_code_ = state_code;
        redirect_ = true;
        redirect_url_ = url;
    }

    bool Close() {
        std::string_view header = "Connection";
        if (HasHeader(header) && GetHeader(header) == "keep-alive") {
            return false;
        }
        return true;
    }
};