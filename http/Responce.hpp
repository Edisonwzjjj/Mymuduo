#pragma once

#include "Util.hpp"

class Response {
public:
    int state_code_{200};
    bool redirect_{false};

    std::string body_;
    std::string redirect_url_;
    std::unordered_map<std::string, std::string> headers_;

public:
    Response() = default;
    explicit Response(int state_code): state_code_(state_code) {}

    void Reset() {

    }

    void SetHeader(const std::string &key, const std::string &val) {
        headers_.insert(std::make_pair(key, val));
    }

    bool HasHeader(const std::string &key) const {
        auto it = headers_.find(key);
        return it != headers_.end();
    }

    std::string GetHeader(std::string &key) {
        bool st  = HasHeader(key);
        if (st) {
            return headers_[key];
        }
        return "";
    }

    void SetContent(const std::string &body, const std::string &type) {
        body_ = body;
        std::string key("Content-Type");
        SetHeader(key, type);
    }

    void SetRedirect(std::string &url, int state_code = 302) {
        state_code_ = state_code;
        redirect_ = true;
        redirect_url_ = url;
    }

    bool Close() {
        std::string header = "Connection";
        if (HasHeader(header) && GetHeader(header) == "keep-alive") {
            return false;
        }
        return true;
    }
};