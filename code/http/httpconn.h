#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn{
public:
    HttpConn();

    ~HttpConn();

    //初始化函数，接受一个套接字描述符和一个sockaddr_in结构，用于指定连接的地址
    void init(int sockFd, const sockaddr_in& addr);

    //读取数据
    ssize_t read(int* saveError);

    //写入数据
    ssize_t write(int* saveError);

    //关闭连接
    void Close();

    //返回连接的文件描述符
    int GetFd() const;

    //返回连接的端口
    int GetPort() const;

    //返回连接的IP地址
    const char* GetIP() const;

    //返回连接的 sockaddr_in 地址
    sockaddr_in GetAddr() const;

    //处理连接
    bool process();

    //获取待写入的字节数
    int ToWriteBytes(){
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool IsKeepAlive() const{
        return request_.IsKeepAlive();
    }

    static bool isET; //是否采用边缘触发模式
    static const char* srcDir;  //源文件目录
    static std::atomic<int> userCount; //用户计数，采用原子操作保证线程安全

private:
    int fd_;
    struct sockaddr_in addr_;

    bool isClose_;

    int iovCnt_; //IO向量计数
    struct iovec iov_[2]; //IO向量数组，用于读写操作。

    Buffer readBuff_; //读缓冲区,用于存储从客户端接收到的数据
    Buffer writeBuff_;//写缓冲区，用于存储即将发送到客户端的数据

    HttpRequest request_; //HTTP请求对象
    HttpResponse response_; //HTTP响应对象


};


#endif