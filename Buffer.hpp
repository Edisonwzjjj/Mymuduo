//
// Created by ZIJUN WANG on 2/2/2024.
//
#pragma once
#ifndef MYMUDUO_BUFFER_HPP
#define MYMUDUO_BUFFER_HPP

#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <iostream>

#define MAX_BUF_SIZE 1024

class Buffer {
private:
    std::vector<char> buf_;
    uint64_t read_idx_{0};
    uint64_t write_idx_{0};
public:
    explicit Buffer(int size = MAX_BUF_SIZE) : buf_(size) {}

    auto Begin() -> char * {
        return &*buf_.begin();
    }

    auto WritePos() -> char * {
        return Begin() + write_idx_;
    }

    auto ReadPos() -> char * {
        return Begin() + read_idx_;
    }

    auto TailSpace() -> uint64_t {
        return buf_.size() - write_idx_;
    }

    auto HeadSpace() const -> uint64_t {
        return read_idx_;
    }

    auto ReadableSize() const -> uint64_t {
        assert(write_idx_ - read_idx_ >= 0);
        return write_idx_ - read_idx_;
    }

    void MoveRead(uint64_t len) {
        assert(len <= ReadableSize());
        read_idx_ += len;
    }

    void MoveWrite(uint64_t len) {
        assert(len <= TailSpace());
        write_idx_ += len;
    }

    void EnsureWriteSpace(uint64_t len) {
        if (TailSpace() >= len) {
            return;
        }
        if (len <= TailSpace() + HeadSpace()) {
            uint64_t rsz = ReadableSize();
            std::copy(ReadPos(), ReadPos() + rsz, Begin());
            read_idx_ = 0;
            write_idx_ = rsz;
        } else {
            buf_.resize(write_idx_ + len);
        }
    }

    void Write(const void *data, uint64_t len) {
        if (len) {
            EnsureWriteSpace(len);
            auto d = static_cast<const char *>(data);
            std::copy(d, d + len, WritePos());
            MoveWrite(len);
        }
    }

    void Write(const std::string& str) {
        Write(str.c_str(), str.size());
    }


    void Read(char *buf, uint64_t len) {
        assert(len <= ReadableSize());
        std::copy(ReadPos(), ReadPos() + len, buf);
        MoveRead(len);
    }

    auto Read(uint64_t len) -> std::string {
        //pre allocate memory otherwise do nothing
        std::string str(len, '\0');
        Read(str.data(), len);
        return str;
    }

    auto GetLine() -> std::string {
        auto pos = std::find(ReadPos(), WritePos(), '\n');
        if (!pos) {
            return {};
        }
        return Read(pos - ReadPos() + 1);
    }

    void Clear() {
        read_idx_ = 0;
        write_idx_ = 0;
    }
};

#endif //MYMUDUO_BUFFER_HPP
