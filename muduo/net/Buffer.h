#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H
#include <vector>
#include <string>
namespace mulib{
    namespace net{
        class Buffer{
        public:
            const static size_t kCheapPrepend = 8;
            const static size_t kInitialSize = 1024;
            Buffer();
            size_t readableBytes() const;
            size_t writableBytes() const;
            size_t prependableBytes() const;
            void swap(Buffer &rhs);
            const char *peek() const; // 返回当前可读数据的指针
            void retrieve(size_t len);
            void retrieveUntil(const char *end);
            void retrieveAll();
            std::string retrieveAllAsString();
            std::string retrieveAsString(size_t len);
            void append(const char* data,size_t len);
            void ensureWritableBytes(size_t len);
            const char* findn();
            std::string returnstring();
            char* beginWrite();
            const char *beginWrite() const;
            void hasWritten(size_t len);

            void prepend(const void *data, size_t len);
            size_t internalCapacity() const;
            ssize_t readFd(int, int *saveErrno);
        private:
            char *begin();
            const char *begin() const;
            void makeSpace(size_t len);
            std::vector<char> buffer_;
            size_t readerIndex_; // 读指针，指向当前可读数据的起始位置
            size_t writerIndex_; // 写指针，指向当前可写数据的起始位置
        };
    }
}
#endif
