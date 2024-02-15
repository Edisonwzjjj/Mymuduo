#pragma once

#include "Util.hpp"

class Request {
public:
    std::string method_;
    std::string path_;
    std::string version_{"HTTP/1.1"};
    std::string body_;

    std::smatch match_;

    std::unordered_map<std::string_view, std::string_view> headers_;
    std::unordered_map<std::string_view, std::string_view> params_;

public:
    Request() = default;

    void Reset() {

    }

    void SetHeader(const std::string &key, const std::string &val) {
        headers_.insert(std::make_pair(key, val));
    }

    bool HasHeader(std::string_view &key) const {
        auto it = headers_.find(key);
        return it != headers_.end();
    }

    std::string_view GetHeader(std::string_view &key) const {
        bool st  = HasHeader(key);
        if (st) {
            return headers_[key];
        }
        return "";
    }

    void SetParam(std::string_view &key, std::string_view &val) {
        params_.insert(std::make_pair(key, val));
    }

    bool HasParam(std::string_view &key) {
        auto it = params_.find(key);
        return it != params_.end();
    }

    std::string_view GetParam(std::string_view &key) {
        bool st  = HasParam(key);
        if (st) {
            return params_[key];
        }
        return "";
    }

    size_t ContentLength() const {
        // Content-Length: 1234\r\n
        std::string_view header = "Content-Length";
        bool ret = HasHeader(header);
        if (!ret) {
            return 0;
        }
        auto str = GetHeader(header);
        return std::stol(str.data());
    }

    // 短连接
    bool Close() const {
        // 没有Connection字段，或者有Connection但是值是close，则都是短链接，否则就是长连接
        std::string_view header = "Connection";
        if (HasHeader(header) && GetHeader(header) == "keep-alive") {
            return false;
        }
        return true;
    }
};