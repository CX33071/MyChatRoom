#include "Buffer.h"
#include <assert.h>
#include <algorithm>
#include <sys/uio.h>

using namespace mulib::net;

Buffer::Buffer() :
buffer_(kCheapPrepend + kInitialSize),
readerIndex_(kCheapPrepend),
writerIndex_(kCheapPrepend){
    assert(readableBytes() == 0);
    assert(writableBytes() == kInitialSize);
    assert(prependableBytes() == kCheapPrepend);
}
size_t Buffer::readableBytes() const{
    return writerIndex_ - readerIndex_;
}

size_t Buffer::writableBytes() const{
    return buffer_.size() - writerIndex_;
}
size_t Buffer::prependableBytes() const{
    return readerIndex_;
}
void Buffer::swap(Buffer &rhs){
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
}
char *Buffer::begin(){
    return &*buffer_.begin();
}

const char *Buffer::begin() const{
    return &*buffer_.begin();
}

const char *Buffer::peek() const{
    return begin() + readerIndex_;
}
void Buffer::retrieve(size_t len){
    assert(len <= readableBytes());
    readerIndex_ += len;
}
void Buffer::retrieveUntil(const char* end){
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
}
void Buffer::retrieveAll(){
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}
std::string Buffer::retrieveAllAsString(){
    return retrieveAsString(readableBytes());
}
std::string Buffer::retrieveAsString(size_t len){
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
}
void Buffer::ensureWritableBytes(size_t len){
    if(writableBytes() < len){
        makeSpace(len);
    }
    assert(writableBytes() >= len);
}
void Buffer::append(const char *data, size_t len){
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
}
char *Buffer::beginWrite(){
    return begin() + writerIndex_;
}
const char *Buffer::beginWrite() const{
    return begin() + writerIndex_;
}
void Buffer::hasWritten(size_t len){
    writerIndex_ += len;
}
void Buffer::prepend(const void *data, size_t len){
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char *_data = static_cast<const char *>(data);
    std::copy(_data, _data + len, begin() + readerIndex_);
}
size_t Buffer::internalCapacity() const{
    return buffer_.capacity();
}
// 从文件描述符（通常是 socket）读取数据，填充到 Buffer 中（readfd)//
ssize_t Buffer::readFd(int fd, int *saveErrno){
    char extrabuf[65535];
    iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf) ? 2 : 1);
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0){
        *saveErrno = errno;
    }
    else if (static_cast<size_t>(n) <= writable){
        writerIndex_ += n;
    }
    else{
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}
void Buffer::makeSpace(size_t len){
    if(writableBytes() + prependableBytes() < len + kCheapPrepend){
        buffer_.resize(writerIndex_ + len);
    }
    else{
        assert(kCheapPrepend < readerIndex_);
        std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
        size_t readable = readableBytes();
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
        assert(readable == readableBytes());
    }
}
const char* Buffer::findn() {
    const char* start = peek();
    const char* end = beginWrite();

    const char* p = std::find(start, end, '\n');
    return (p == end) ? nullptr : p;
}
std::string Buffer::returnstring() {
    const char* p = findn();
    assert(p != nullptr);
    size_t len = p - peek();
    std::string res(peek(), len);
    retrieve(len + 1); 
    return res;
}