#pragma once
#ifndef MYMUDUO_LOG_HPP
#define MYMUDUO_LOG_HPP

#include <chrono>
#include <iostream>
#include <cstdarg>
#include <source_location>
#include <format>

using std::cout;
using std::endl;

enum class Level {
    DBG,
    INFO,
    ERROR
};

class Logger {
private:
    Level level_{Level::DBG};
    static Logger logger_;
private:
    Logger() = default;


public:
    ~Logger() = default;

    static Logger GetLogger() {
        return logger_;
    }

    void SetLevel(const Level level) {
        level_ = level;
    }

    void Log(std::source_location &sl, std::string_view message) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm *lt = std::localtime(&t);

        std::cout << std::format("{} {} {}, file name is: {}, line no is: {}, MESSAGE: {}",
                                 lt->tm_hour, lt->tm_min, lt->tm_sec, sl.file_name(), sl.line(),
                                 message);
    }
};

Logger Logger::logger_{};

#endif //MYMUDUO_LOG_HPP
