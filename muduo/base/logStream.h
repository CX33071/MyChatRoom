#ifndef MUDUO_BASE_LOGSTREAM_H
#define MUDUO_BASE_LOGSTREAM_H

#include <iostream>
#include <sstream>
#include "noncopyable.h"
#define MAX_STRING_SIZE 4096
namespace mulib{
    namespace base{
        class LogStream : noncopyable
        {
        public:
            template <typename T>
            LogStream &operator<<(const T &val);
            LogStream &operator<<(const bool &val);
            LogStream &operator<<(std::ostream &(*manip)(std::ostream &));

            void append(const std::string &s, size_t pos, int n);
            void append(const std::string &s);

            const std::string &str() const;
            void reset();

        private:
            bool isMaxString(){
                if (buffer_.size() < MAX_STRING_SIZE)
                {
                    return true;
                }
                std::cout << "buffer_'size reaching the maximum" << std::endl;
                return false;
            }
            std::string buffer_;

        };
    }
}
using namespace mulib::base;
template <typename T>
LogStream &LogStream::operator<<(const T &val)
{
    std::ostringstream oss;
    oss << val;
    if (isMaxString())
    {
        buffer_ += oss.str();
    }
    return *this;
}
#endif
