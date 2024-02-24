#include "httpconn.h"

using namespace std;

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn(){
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
}

HttpConn::~HttpConn(){
    Close();
}

void HttpConn::init(int fd, const sockaddr_in& addr){
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close(){
    response_.UnmapFile();
    if(isClose_ == false){
        isClose_ = true;
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);

    }
}

int HttpConn::GetFd() const {
    return fd_;
}

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const{
    return addr_.sin_port;
}

//分散读，将fd_上的数据读入到readBuff_对象的私有变量 buffer_中
ssize_t HttpConn::read(int* saveErrno){
    ssize_t len = -1;
    do{
        //调用readBuff_的成员方法ReadFd 分散读
        len = readBuff_.ReadFd(fd_, saveErrno);
        if(len <= 0){
            break;
        }
    }while(isET);
    return len;
}


//分散写
ssize_t HttpConn::write(int* saveErrno){
    ssize_t len = -1;
    do{
        len = writev(fd_, iov_, iovCnt_);
        if( len <= 0 ){
            *saveErrno = errno;
            break;
        }

        if( iov_[0].iov_len + iov_[1].iov_len == 0) { break; }/*传输结束*/
        //写入的字节数 len 大于 iov_[0].iov_len，说明iov_[0]写满，需要更新iov_[1]和写缓冲区。
        else if(static_cast<size_t>(len) > iov_[0].iov_len){

            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= ( len - iov_[0].iov_len);
            if( iov_[0].iov_len){
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        //如果写入的字节数 len 小于等于 iov_[0].iov_len，说明iov_[0]未写满，只需更新iov_[0]和写缓冲区。
        else{
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len  -= len;
            writeBuff_.Retrieve(len);
        }
    }while( isET || ToWriteBytes() > 10240);
    return len;
}


bool HttpConn::process(){
    request_.Init();
    if( readBuff_.ReadableBytes() <= 0){
        return false;
    }
    //解析 HTTP 请求
    else if( request_.parse(readBuff_)){
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    }
    //解析失败，状态码为400
    else{
        response_.Init(srcDir, request_.path(), false, 400);
    }
    //生成 HTTP 响应存入 写缓冲区中
    response_.MakeResponse(writeBuff_);

    /* 响应头
        iov_[0].iov_base：指向写缓冲区中待写入数据的起始位置。
        iov_[0].iov_len：表示待写入的数据长度。
     */

    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 文件 
                
        iov_[1].iov_base：存放额外的数据，指向文件内容首地址。
        iov_[1].iov_len：表示额外数据的长度，即文件内容长度
    */

   //检查是否有文件内容需要写入响应。
    if(response_.FileLen() > 0 && response_.File()){
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len =response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}