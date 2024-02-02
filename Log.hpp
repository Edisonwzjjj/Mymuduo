//
// Created by ZIJUN WANG on 2/2/2024.
//

#ifndef MYMUDUO_LOG_HPP
#define MYMUDUO_LOG_HPP
#include <ctime>
#include <iostream>
#include <cstdarg>
#include <chrono>
#include <iomanip>
#define DBG 0
#define INFO 1
#define ERROR 2
#define DEFAULT_LEVEL DBG

//#define LOG(level, format, ...) do { \
//   if (level < DEFAULT_LEVEL) break;\
//    time_t t = time(0);             \
//    struct tm *local_time = localtime(&t); \
//    char tmp[32]{};                 \
//    strftime(tmp, 31, "%H: %M: %S", local_time); \
//    fprintf(stdout, "[%s %s:%d] ", format "\n", tmp, __FILE__, __LINE__, ##__VA_ARGS__); \
//} while (0)

 #define LOG(level, format, ...) do { \
     if (level < DEFAULT_LEVEL) break; \
     auto now = std::chrono::system_clock::now(); \
     auto time_t = std::chrono::system_clock::to_time_t(now); \
     std::tm local_time = *std::localtime(&time_t); \
     auto time_str = std::put_time(&local_time, "%H:%M:%S"); \
     std::cout << "[ " << time_str << " " << __FILE__ << ":" << __LINE__ << " ] " \
               << format << std::endl; \
 } while (0)

#define INFO_LOG(format, ...) LOG(INFO, format, ##__VA_ARGS__)
#define DBG_LOG(format, ...) LOG(DBG, format, ##__VA_ARGS__)
#define ERROR_LOG(format, ...) LOG(ERROR, format, ##__VA_ARGS__)


#endif //MYMUDUO_LOG_HPP
