#pragma once

#include "EventLoop.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>

class LoopThread {
private:
    /*用于实现_loop获取的同步关系，避免线程创建了，但是_loop还没有实例化之前去获取_loop*/
    std::mutex _mutex;          
    std::condition_variable _cond;   
    EventLoop *_loop;       
    std::thread _thread;    
private:
    /*实例化 EventLoop 对象，唤醒_cond上有可能阻塞的线程，并且开始运行EventLoop模块的功能*/
    void ThreadEntry() {
        EventLoop loop;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _loop = &loop;
            _cond.notify_all();
        }
        loop.Start();
    }
public:
    /*创建线程，设定线程入口函数*/
    LoopThread():_loop(nullptr), _thread(std::thread(&LoopThread::ThreadEntry, this)) {}
    /*返回当前线程关联的EventLoop对象指针*/
    EventLoop *GetLoop() {
        EventLoop *loop = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cond.wait(lock, [&](){ return _loop != nullptr; });
            loop = _loop;
        }
        return loop;
    }
};