#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "blockqueue.h"
#include "../buffer/buffer.h"


class Log{
public:
    void init( int level, const char* path = "./log", const char* suffix = ".log", int maxQueueCapacity = 1024);
    //单例模式
    static Log* Instance();
    static void FlushLogThread();

    void write(int level,const char* format, ...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }

private:
    //构造函数为私有，防止外部调用
    Log();
    void AppendLogLevelTitle_(int level);
    virtual ~Log();
    void AsyncWrite_();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* path_;
    const char* suffix_;

    int MAX_LINES_;

    int lineCount_;
    int toDay_;

    bool isOpen_;

    Buffer buff_;
    int level_;
    bool isAsync_;

    //文件指针，指向日志文件
    FILE* fp_;
    //阻塞队列
    std::unique_ptr<BlockDeque<std::string>> deque_;
    std::unique_ptr<std::thread> writeThread_;
    //互斥锁 mtx_ 保护日志写入的线程安全
    std::mutex mtx_;
};

#define LOG_BASE(level,format,...)\
    do{\
        Log* log = Log::Instance();\
        if( log->IsOpen() && log->GetLevel() <= level ){\
            log->write(level,format,##__VA_ARGS__);\
            log->flush();\
        }\
    }while(0);

#define LOG_DEBUG(format,...) do {LOG_BASE(0, format, #__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif