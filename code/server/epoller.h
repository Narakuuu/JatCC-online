#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

class Epoller{
public:
    explicit Epoller( int maxEvent = 1024);
    ~Epoller();

    //向 epoll 中添加一个文件描述符及其关注的事件。
    bool AddFd( int fd, uint32_t events);

    //修改已经在 epoll 中注册的文件描述符关注的事件。
    bool ModFd( int fd, uint32_t events);

    //从 epoll 中删除一个文件描述符。
    bool DelFd( int fd );

    //等待事件的发生，返回发生事件的文件描述符数量
    int Wait( int timeoutMs = -1);

    //返回在数组中索引为 i 的事件的文件描述符
    int GetEventFd(size_t i ) const;

    //回在数组中索引为 i 的事件的具体事件类型
    uint32_t GetEvents(size_t i) const; 

private:
    int epollFd_;
    std::vector<struct epoll_event> events_;

};




#endif