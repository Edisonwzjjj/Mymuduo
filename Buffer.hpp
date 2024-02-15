#pragma once

#include "Log.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <iostream>

#define BUFFER_DEFAULT_SIZE 1024

class Buffer {
private:
    std::vector<char> buf_; //使用vector进行内存空间管理
    uint64_t read_idx_{0}; //读偏移
    uint64_t write_idx_{0}; //写偏移
public:
    Buffer() : buf_(BUFFER_DEFAULT_SIZE) {}

    auto Begin() -> char * { return &*buf_.begin(); }

    auto WritePosition() -> char * { return Begin() + write_idx_; }

    auto ReadPosition() -> char * { return Begin() + read_idx_; }

    //获取缓冲区末尾空闲空间大小--写偏移之后的空闲空间, 总体空间大小减去写偏移
    uint64_t TailIdleSize() { return buf_.size() - write_idx_; }

    //获取缓冲区起始空闲空间大小--读偏移之前的空闲空间
    uint64_t HeadIdleSize() const { return read_idx_; }

    //获取可读数据大小 = 写偏移 - 读偏移
    uint64_t ReadableSize() const { return write_idx_ - read_idx_; }

    //将读偏移向后移动
    void MoveReadOffset(uint64_t len) {
        if (!len) return;
        //向后移动的大小，必须小于可读数据大小
        assert(len <= ReadableSize());
        read_idx_ += len;
    }

    //将写偏移向后移动
    void MoveWriteOffset(uint64_t len) {
        //向后移动的大小，必须小于当前后边的空闲空间大小
        assert(len <= TailIdleSize());
        write_idx_ += len;
    }

    //确保可写空间足够（整体空闲空间够了就移动数据，否则就扩容）
    void EnsureWriteSpace(uint64_t len) {
        //如果末尾空闲空间大小足够，直接返回
        if (TailIdleSize() >= len) { return; }
        //末尾空闲空间不够，则判断加上起始位置的空闲空间大小是否足够, 够了就将数据移动到起始位置
        if (len <= TailIdleSize() + HeadIdleSize()) {
            //将数据移动到起始位置
            uint64_t rsz = ReadableSize();//把当前数据大小先保存起来
            std::copy(ReadPosition(), ReadPosition() + rsz, Begin());//把可读数据拷贝到起始位置
            read_idx_ = 0;    //将读偏移归0
            write_idx_ = rsz;  //将写位置置为可读数据大小， 因为当前的可读数据大小就是写偏移量
        } else {
            //总体空间不够，则需要扩容，不移动数据，直接给写偏移之后扩容足够空间即可
            DBG_LOG("RESIZE %ld", write_idx_ + len);
            buf_.resize(write_idx_ + len);
        }
    }


    void Write(const void *data, uint64_t len) {
        //1. 保证有足够空间，2. 拷贝数据进去
        if (len == 0) return;
        EnsureWriteSpace(len);
        auto d = static_cast<const char *>(data);
        std::copy(d, d + len, WritePosition());
        MoveWriteOffset(len);
    }


    void Read(void *buf, uint64_t len) {
        assert(len <= ReadableSize());
        std::copy(ReadPosition(), ReadPosition() + len, (char *) buf);
        MoveReadOffset(len);
    }

public:
    void Write(std::string_view &str) {
        return Write(str.data(), str.size());
    }

    std::string Read(uint64_t len) {
        //要求要获取的数据大小必须小于可读数据大小
        assert(len <= ReadableSize());
        std::string str;
        str.resize(len);
        Read(str.data(), len);
        return str;
    }

    std::string GetLine() {
        auto pos = std::find(ReadPosition(), ReadPosition() + ReadableSize() + 1, '\n');
        if (!pos) {
            return "";
        }
        return Read(pos - ReadPosition() + 1);
    }

    void Clear() {
        read_idx_ = 0;
        write_idx_ = 0;
    }
};

