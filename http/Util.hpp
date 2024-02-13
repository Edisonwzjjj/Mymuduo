#pragma once

#include "../TcpServer.hpp"
#include <regex>
#include <fstream>
#include <sys/stat.h>

std::unordered_map<int, std::string_view> STATE_MSG{
        {200, "OK"},
        {301, "Moved Permanently"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {500, "Internal Server Error"}
};

std::unordered_map<std::string_view, std::string_view > MIME_MSG {
        {".html",     "text/html"},
        {".txt",        "text/plain"},
        {".json",       "application/json"},
};

class Util {
public:
    static std::vector<std::string_view> Split(std::string_view &str, std::string_view &sep) {
        std::vector<std::string_view> res;

        size_t offset = 0;
        while (offset < str.size()) {
            int pos = str.find(sep, offset);
            if (pos == std::string::npos) {
                res.push_back(str.substr(offset));
                break;
            }
            if (pos == offset) {
                offset = pos + sep.size();
                continue;
            }
            res.push_back(str.substr(offset, pos - offset));
            offset = pos + sep.size();
        }

        return res;
    }

    static bool ReadFile(std::string_view &file_name, std::string &buf) {
        std::ifstream ifs(file_name.data(), std::ios::binary);
        if (!ifs.is_open()) {
            ERR_LOG("OPEN %s FILE FAILED!", file_name.data());
            return false;
        }
        size_t fsize = 0;
        ifs.seekg(0, std::ifstream::end);
        fsize = ifs.tellg();
        ifs.seekg(0, std::ifstream::beg);
        buf.resize(fsize);

        ifs.read(buf.data(), fsize);
        if (!ifs.good()) {
            ERR_LOG("READ %s FILE FAILED!", file_name.data());
            ifs.close();
            return false;
        }

        ifs.close();
        return true;
    }

    static bool WriteFile(std::string_view &file_name, std::string_view src) {
        std::ofstream ofs(file_name.data(), std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) {
            ERR_LOG("OPEN %s FILE FAILED!", file_name.data());
            return false;
        }

        ofs.write(src.data(), src.size());
        if (!ofs.good()) {
            ERR_LOG("WRITE %s FILE FAILED!", file_name.data());
            ofs.close();
            return false;
        }

        ofs.close();
        return true;
    }

    // %HH 空格转+    .-_~ 字母数字不编码
    static std::string UrlEncode(std::string_view &url) {
        std::string res;
        for (auto &c: url) {
            if (c == '.' || c == '-' || c == '_' || c == '~' || isalnum(c)) {
                res += c;
            } else if (c == ' ') {
                res += '+';
            } else {
                char tmp[4]{};
                snprintf(tmp, 4, "%%%02X", c);
                res += tmp;
            }
        }

        return res;
    }

    static char HEXTOI(const char &c) {
        if (c >= '0' && c <= '9') {
            return c - '0';
        } else if (c >= 'a' && c <= 'z') {
            return c - 'a' + 10;
        } else if (c >= 'A' && c <= 'Z') {
            return c - 'A' + 10;
        }
    }

    static std::string UrlDecode(std::string_view &url) {
        std::string res;
        for (int i = 0; i < url.size(); ++i) {
            auto c = url[i];
            if (c == '+') {
                res += ' ';
            } else if (c == '%' && i + 2 < url.size()) {
                char v1 = HEXTOI(url[i + 1]);
                char v2 = HEXTOI(url[i + 2]);
                char v = v1 * 16 + v2;
                res += v;
                i += 2;
            } else {
                res += c;
            }
        }

        return res;
    }

    static std::string_view StateDesc(int code) {
        auto it = STATE_MSG.find(code);
        if (it == STATE_MSG.end()) {
            return "";
        }
        return it->second;
    }

    static std::string_view ExtMime(std::string_view &filename) {
        size_t pos = filename.find_last_of('.');
        if (pos == std::string::npos) {
            return "application/octet-stream";
        }

        std::string_view ext = filename.substr(pos);
        auto it = MIME_MSG.find(ext);
        if (it == MIME_MSG.end()) {
            return "application/octet-stream";
        }
        return it->second;
    }

    static bool IsDir(std::string_view &filename) {
        struct stat st{};
        int res = stat(filename.data(), &st);
        if (res < 0) {
            return false;
        }
        return S_ISDIR(st.st_mode);
    }

    static bool IsRegular(std::string_view &filename) {
        struct stat st{};
        int res = stat(filename.data(), &st);
        if (res < 0) {
            return false;
        }
        return S_ISREG(st.st_mode);
    }

    static bool ValidPath(std::string_view &path) {
        int level{0};
        std::string_view sep = "\\";
        auto tmp = Split(path, sep);
        for (auto &p: tmp) {
            if (p == "..") {
                --level;
                if (!level) {
                    return false;
                }
            }
            ++level;
        }

        return true;
    }
};