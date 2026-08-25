#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H
#include "logStream.h"
#include "Timestamp.h"
#include <iostream>
#include <time.h>
#include <cstring>
#include "noncopyable.h"
#define LOG_TRACE                                          \
    if (mulib::base::Logger::logLevel() <= mulib::base::Logger::TRACE) \
    mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                          \
    if (mulib::base::Logger::logLevel() <= mulib::base::Logger::DEBUG) \
    mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                          \
    if (mulib::base::Logger::logLevel() <= mulib::base::Logger::INFO) \
    mulib::base::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::WARN).stream()
#define LOG_ERROR mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::ERROR).stream()
#define LOG_FATAL mulib::base::Logger(__FILE__, __LINE__, mulib::base::Logger::FATAL).stream()
#define LOG_SYSERR mulib::base::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL mulib::base::Logger(__FILE__, __LINE__, true).stream()
namespace mulib
{
    namespace base
    {
        class Logger{
        public:
            enum LogLevel{
                TRACE, // 追踪程序的详细运行过程
                DEBUG, // 调试信息
                INFO,  // 一般性信息，如程序启动、配置加载成功等
                WARN,  // 警告，表示程序出现了轻微异常或潜在问题，但还能运行
                ERROR, // 错误
                FATAL  // 致命错误
            };
            class SourceFile{
            public:
                template <int N>
                SourceFile(const char (&arr)[N]); // 用于记录 __FILE__
                explicit SourceFile(const char *filename);
                const char *data_;
                int size_;
            };
            Logger();
            Logger(SourceFile file, int line);
            Logger(SourceFile file, int line, LogLevel level);
            Logger(SourceFile file, int line, LogLevel level, const char *func);
            Logger(SourceFile file, int line, bool toAbort); // SYSFATAL 日志
            LogStream &stream(); // 获取流式日志输入
            static LogLevel logLevel();
            void setLogLevel(LogLevel level);
            ~Logger();
        private:
            class Impl{
            public:
                Impl(LogLevel level, int savedErrno, const SourceFile &file, int line);
                void formatTime();
                void formatLevel();
                void finish();

                mulib::base::Timestamp time_;
                LogStream stream_;
                LogLevel level_;
                int line_;
                SourceFile basename_;
            };
            Impl impl_;
        };
    }
}
using namespace mulib::base;
template <int N>
Logger::SourceFile::SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1) // 用于接收固定长度数组的引用
{
    const char *slash = strrchr(data_, '/');
    if (slash)
    {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
    }
}
#endif
