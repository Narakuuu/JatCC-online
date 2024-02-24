#include "httprequest.h"
using namespace std;
namespace fs = std::filesystem;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
            "/index", "/helper", "/compile", "/show",
             "/welcome", "/video", "/picture", };

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/compile.html", 0},  };

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.ReadableBytes() <= 0) {
        return false;
    }
    while(buff.ReadableBytes() && state_ != FINISH) {
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.Peek(), lineEnd);
        switch(state_)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)) {
                return false;
            }
            ParsePath_();
            break;    
        case HEADERS:
            ParseHeader_(line);
            if(buff.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        //如果当前处理的行是缓冲区中最后一行数据
        if(lineEnd == buff.BeginWrite()) { 
            //清空缓冲区内容，防止遗留数据影响下次解析请求
            buff.RetrieveAll();
            break; 
        }
        //如果缓冲区内还有数据，清除刚刚处理的一行数据
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

//解析请求的路径
void HttpRequest::ParsePath_() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        //遍历默认 HTML 路径集合
        for(auto &item: DEFAULT_HTML) {
            //如果匹配
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const string& line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    //使用正则表达式匹配请求行
    if(regex_match(line, subMatch, patten)) {
        /*
            以GET /index HTTP/1.1为例
            subMatch[0] 将存储整个匹配结果，即 "GET /index HTTP/1.1
            subMatch[1] 是 GET
            subMatch[2] 是 /index
            subMatch[3] 是 1.1
        */   
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        //设置解析状态为 HEADERS 表示解析请求头
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}


//解析请求头 提取键值对
void HttpRequest::ParseHeader_(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        /*
            以请求头Content-Type: application/json为例
            subMatch[1]是 "Content-Type"，即匹配到的键。
            subMatch[2]是 "application/json"，即匹配到的值。
        */
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        //匹配失败，表示请求头解析结束，将解析状态设置为 BODY
        state_ = BODY;
    }
}

//解析请求体
void HttpRequest::ParseBody_(const string& line) {
    body_ = line; //将请求主体内容存储到 body_ 中
    ParsePost_(); //// 解析 POST 请求的表单数据
    state_ = FINISH; // // 设置解析状态为 FINISH，表示请求解析完成
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

void writeParserResult(string res){
    cout<<"调用 writeParseResult"<<endl;
    fs::path filepath= "/home/vijay/Desktop/VijayWebServer/WebServer_Vijay/resources/ret.txt";//resources/ret.txt code/http/httprequest.cpp
    ofstream output{ filepath };

    if (!output.is_open()) {
        LOG_ERROR("Error: Unable to open or create the file")
        return;
    }
        // 写入字符串到文件
    output << res << endl;;
    output.close();
}


void HttpRequest::ParsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        std::string codeText = ParseFromUrlencoded_();
        if(DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            //如果请求来自 compile 页面
            if(tag == 0) {
                if(post_.count("inputText")){
                    // cout<<"codeText内容:" << codeText<<endl;
                    const char * codeCstring =  codeText.c_str();
                    cout<<"codeCstring内容:"<<codeCstring<<endl;
                    string result = Jatcc(codeCstring);
                    cout<<"result:"<<endl;
                    cout<<result<<endl;
                    writeParserResult(result);
                    path_ = "/compiled.html";
                }

            }
        }
    }   
}






//解析URL 数据
const std::string HttpRequest::ParseFromUrlencoded_()
{   
    std::string codeText;
    std::string key, value;
    char hex[3] = {0};
    if(body_.size() == 0) { return ""; }
    int n = body_.size();
    int i = 0, j = 0;
    for(; i < n; i++){
        char ch = body_[i];
        if( ch == '='){
            key = body_.substr(j, i - j);
            j = i + 1;
        }
        if( ch == '%' && i+2 < n){
            hex[0] = body_[i+1];
            hex[1] = body_[i+2];
            i += 2;
            codeText += static_cast<char>(std::stoi(hex, nullptr, 16));
        }
        else if( ch == '+'){
            codeText +=' ';
        }
        else{
            codeText += ch;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
    return codeText.substr(10, codeText.size());
}


std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}
std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}