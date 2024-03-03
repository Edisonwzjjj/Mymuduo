#pragma once

#include "Thread.hpp"

class LoopThreadPool {
private:
    int _thread_count;
    int _next_idx;
    EventLoop *_baseloop;
    std::vector<LoopThread*> _threads;
    std::vector<EventLoop *> _loops;
public:
    explicit LoopThreadPool(EventLoop *baseloop):_thread_count(0), _next_idx(0), _baseloop(baseloop) {}
    void SetThreadCount(int count) { _thread_count = count; }
    void Create() {
        if (_thread_count > 0) {
            _threads.resize(_thread_count);
            _loops.resize(_thread_count);
            for (int i = 0; i < _thread_count; i++) {
                _threads[i] = new LoopThread();
                _loops[i] = _threads[i]->GetLoop();
            }
        }
    }
    EventLoop *NextLoop() {
        if (_thread_count == 0) {
            return _baseloop;
        }
        _next_idx = (_next_idx + 1) % _thread_count;
        return _loops[_next_idx];
    }
};