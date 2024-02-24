#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/stat.h> //stat
#include <sys/mman.h> //mmap munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse{
public:
    HttpResponse();
    ~HttpResponse();

    //初始化HttpResponse对象，设置相应的基本信息：源目录、请求路径、是否保持连接以及状态码
    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);

    //生成完整的HTTP响应，状态行、响应头和响应主体，将结果存储到 Buffer 中。
    void MakeResponse(Buffer& buff);

    //取消映射文件，释放相应资源
    void UnmapFile();

    //返回映射文件的内容
    char* File();

    //返回映射文件的长度
    size_t FileLen() const;

    //生成错误信息的响应主体
    void ErrorContent(Buffer& buffer, std::string message);

    //返回响应的状态码
    int Code() const { return code_; }

private:

    //添加HTTP响应的状态行，包括状态码、状态描述和 HTTP 版本
    void AddStateLine_(Buffer& buff);

    //添加HTTP响应的头部信息，包括响应时间、服务器信息、内容类型等
    void AddHeader_(Buffer& buff);

    //添加HTTP响应的主体内容吗，包括文件内容或错误信息等
    void AddContent_(Buffer& buff);

    //生成错误页面的 HTML 内容
    void ErrorHtml_();

    //获取文件类型
    std::string GetFileType_();

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char* mmFile_;
    struct stat mmFileStat_;


    //将文件后缀映射到对应的 MIME 类型，如.html文件的MIMIE类型是 text/html
    static const std::unordered_map<std::string,std::string> SUFFIX_TYPE;

    //将 HTTP 状态码映射到对应的状态描述,如状态码200 对应 ok
    static const std::unordered_map<int, std::string> CODE_STATUS;

    //将 HTTP 状态码映射到相应的错误页面路径，如状态码404对应页面路径 /404.html
    static const std::unordered_map<int, std::string> CODE_PATH;
};




#endif