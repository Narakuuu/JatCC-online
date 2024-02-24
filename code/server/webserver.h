#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/threadpool.h"
#include "../http/httpconn.h"


class WebServer{
public:
    //构造函数，初始化web服务器相关参数
    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger,
        int threadNum, bool openLog, int logLevel, int logQueSize
    );

    ~WebServer();

    //启动 web 服务器
    void Start();

private:

    //初始化 Socket
    bool InitSocket_();
    
    //初始化事件模式
    void InitEventMode_(int trigMode);
    
    //添加客户端连接
    void AddClient_(int fd, sockaddr_in addr);

    //处理监听事件
    void DealListen_();
    
    //处理写事件，服务器将响应数据写入套接字
    void DealWrite_(HttpConn* client);
    
    //处理读事件，服务器从套接字中读取客户端发送的请求数据
    void DealRead_(HttpConn* client);

    //发送错误信息
    void SendError_(int fd, const char* info);
    
    //延长连接时间
    void ExtentTime_(HttpConn* client);
    
    //关闭连接
    void CloseConn_(HttpConn* client);

    //读事件回调函数，数据可从文件描述符中读取时，触发读事件
    void OnRead_(HttpConn* client);

    //写事件回调函数，某个文件描述符可写时，触发写事件
    void OnWrite_(HttpConn* client);

    //处理请求的主逻辑
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;

    //文件描述符设置为非阻塞
    static int SetFdNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_; /* 毫秒MS */
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};







#endif