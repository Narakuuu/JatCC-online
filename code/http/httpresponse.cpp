#include "httpresponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
};

HttpResponse::~HttpResponse() {
    UnmapFile();
}

void HttpResponse::Init(const string& srcDir, string& path, bool isKeepAlive, int code){
    assert(srcDir != "");
    if(mmFile_) { UnmapFile(); }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
}

//生成完整的http响应
void HttpResponse::MakeResponse(Buffer& buff) {
    /* 判断请求的资源文件 */

    //如果资源文件不存在或者是一个目录，状态码设置为 404
    if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    //资源文件存在但是没有读权限，状态码设置为403表示禁止访问
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    //否则则设置为 200 表示请求成功
    else if(code_ == -1) { 
        code_ = 200; 
    }
    //code_ 的值生成相应的错误页面内容
    ErrorHtml_();
    // HTTP 响应的状态行添加到 缓冲区 
    AddStateLine_(buff);
    // HTTP 响应的头部信息添加到 缓冲区 
    AddHeader_(buff);
    // HTTP 响应的主体内容添加到 缓冲区
    AddContent_(buff);
}

char* HttpResponse::File() {
    return mmFile_;
}

size_t HttpResponse::FileLen() const {
    return mmFileStat_.st_size;
}

//根据状态码确定是否需要返回自定义的错误页面
void HttpResponse::ErrorHtml_() {
    //如果是预先定义的状态码
    if(CODE_PATH.count(code_) == 1) {
        //获取错误页面路径
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}


void HttpResponse::AddStateLine_(Buffer& buff) {
    string status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        //// 如果找不到对应的状态信息，将 code_ 设置为 400
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    //缓冲区内添加状态行信息
    buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer& buff) {
    buff.Append("Connection: ");
    if(isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else{
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}


void HttpResponse::AddContent_(Buffer& buff) {
    //打开文件获取文件描述符
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if(srcFd < 0) { 
        ErrorContent(buff, "File NotFound!");
        return; 
    }

    LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    
    /* 将文件映射到内存提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");
        return; 
    }
    //// 将映射的起始地址赋值给mmFile_
    mmFile_ = (char*)mmRet;
    close(srcFd);
    // 在响应头中添加Content-length字段，表示响应内容的长度
    buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile() {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

//获取文件类型
string HttpResponse::GetFileType_() {
    string::size_type idx = path_.find_last_of('.');
    //如果找不到点，返回默认类型 "text/plain"
    if(idx == string::npos) {
        return "text/plain";
    }
    //提取文件后缀
    string suffix = path_.substr(idx);

    if(SUFFIX_TYPE.count(suffix) == 1) {
        // 如果在映射表中，返回对应的媒体类型
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

//生成 HTTP 响应中的错误页面内容
void HttpResponse::ErrorContent(Buffer& buff, string message) 
{       
    // 构建HTML格式的错误页面内容
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    // 获取HTTP状态码对应的状态信息
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
