/*
一个HTTP请求分为 请求行(Reqeust Line)、请求头部(Request Headers)、空行(CRLF)、请求主体(Request Body)
    请求行（Request Line）： 包含请求方法、请求的资源路径和 HTTP 协议版本。例如：GET /index.html HTTP/1.1
    请求头部（Request Headers）： 包含关于请求的附加信息，以键值对的形式表示。例如，Content-Type: application/json 表示请求的主体是 JSON 格式。
    空行（CRLF）： 用于分隔请求头部和请求主体的空行，由回车换行符（CRLF）组成。
    请求主体（Request Body）： 通常包含请求的实际数据，例如在 POST 请求中提交的表单数据或 JSON 数据。
    
    以一个表单提交请求为例：
        POST /login HTTP/1.1
        Host: www.example.com
        Content-Type: application/x-www-form-urlencoded

        username=johndoe&password=secretpassword

*/


#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
#include <filesystem>
#include <fstream>

#include "../buffer/buffer.h"
#include "../log/log.h"
// extern "C" 用于处理 C++ 与 C 语法差异
extern "C" {
    #include "../compiler/jatcc.h"
}
class HttpRequest{
public:
    //HTTP 请求的解析状态
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };
    //HTTP 的请求状态码
    enum HTTP_CODE{
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDENT_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    void Init();
    //解析HTTP请求
    bool parse(Buffer& buff);
    
    //获取请求路径
    std::string path() const;
    std::string& path();

    //获取请求方法和协议版本
    std::string method() const;
    std::string version() const;
    //获取POST请求中的数据
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    //判断是否保持连接
    bool IsKeepAlive() const;

private:
    bool isFindCompileButton;
    
    bool getIsFindCompileButton();

    //解析请求行
    bool ParseRequestLine_(const std::string& line);
    
    //解析请求头
    void ParseHeader_(const std::string& line);
    
    //解析请求体
    void ParseBody_(const std::string& line);

    //解析请求路径
    void ParsePath_();
    
    //解析 POST 请求数据
    void ParsePost_();

    //解析URL编码的数据
    const std::string ParseFromUrlencoded_();

    //用户验证函数
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    //解析状态
    PARSE_STATE state_;

    //请求的方法、路径、协议版本和请求体
    std::string method_, path_, version_, body_;
    
    //存放请求头中的字段和值和请求数据中的键值对
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    //默认的HTML内容和HTML标签
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    //辅助函数 十六进制字符转换为整数
    static int ConverHex(char ch);

};




#endif