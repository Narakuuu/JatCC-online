#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>
#include <vector>
#include <atomic>
#include <assert.h>

class Buffer{
public:

    Buffer( int initBuffSize = 1024);

    //使用编译器默认生成的析构函数
    ~Buffer() = default;

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    const char* Peek() const;
    void EnsureWriteable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);

    void RetrieveAll();
    std::string RetrieveAllToStr();

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);
private:
    char* BeginPtr_();
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;

    /*readPos_ 表示当前可读数据的起始位置，是 WriteFd 函数读取数据的位置。
    readPos_之前的是已读但未处理的数据，readPos_ 与 writePos_之间的是未读的数据
    */
    std::atomic<std::size_t> readPos_; 

    //writePos_ 表示当前可写数据的起始位置，是 ReadFd 函数写入数据的位置。
    // readPos_ + ReadableBytes = writePos_
    std::atomic<std::size_t> writePos_;
};

#endif