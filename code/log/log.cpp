#include "log.h"

using namespace std;


Log::Log() {
    lineCount_ = 0;
    isAsync_ = false;
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if(writeThread_ && writeThread_->joinable()) {
        while(!deque_->empty()) {
            deque_->flush();
        };
        deque_->Close();
        writeThread_->join();
    }
    if(fp_) {
        lock_guard<mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

int Log::GetLevel() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}



void Log::init(int level = 1, const char* path, const char* suffix,
    int maxQueueSize) {
    isOpen_ = true;
    level_ = level;
    //检查是否启用异步写入，通过判断传入的 maxQueueSize 是否大于 0
    if(maxQueueSize > 0) {
        isAsync_ = true;
        //deque指针为空，说明尚未初始化异步写入所需的阻塞队列和线程
        if(!deque_) {
            //创建BlockDeque<std::string> 对象 并将指针赋给 newDeque
            unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            //所有权交给deque_
            deque_ = move(newDeque);
            //创建新线程，并将指针赋值给NewThread，同时传入线程函数 FlushLogThread。
            std::unique_ptr<std::thread> NewThread(new thread(FlushLogThread));
            //所有权交给writeThread_
            writeThread_ = move(NewThread);
        }
    } else {
        //不启用异步写入
        isAsync_ = false;
    }

    lineCount_ = 0;

    //获取当前时间戳。
    time_t timer = time(nullptr);
    //转换为本地时间
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;

    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    //格式化生成日志文件名，包括年、月、日等信息
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    //记录当前日期
    toDay_ = t.tm_mday;

    {
        lock_guard<mutex> locker(mtx_);
        buff_.RetrieveAll();
        if(fp_) { 
            flush();
            fclose(fp_); 
        }

        fp_ = fopen(fileName, "a");
        if(fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        } 
        assert(fp_ != nullptr);
    }
}



void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    //获取当前时间
    gettimeofday(&now, nullptr);
    //提取秒数部分
    time_t tSec = now.tv_sec;
    //转换成本地时间
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    /* 检查是否需要创建新的日志文件，条件包括日期变化或者行数达到 MAX_LINES */
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_  %  MAX_LINES == 0)))
    {   
        unique_lock<mutex> locker(mtx_);
        locker.unlock();
        
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        //格式化日期，将结果存储在 tail
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        //日期发生变化，创建新的日志文件名
        if (toDay_ != t.tm_mday)
        {
            //格式化新的日志文件名
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else {
            //如果行数达到 MAX_LINES，则在文件名中添加序号
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_  / MAX_LINES), suffix_);
        }
        //重新加锁
        locker.lock();
        flush();
        fclose(fp_);
        //打开新的日志文件
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }//到达声明周期后，自动释放锁

    {
        //加锁
        unique_lock<mutex> locker(mtx_);
        lineCount_++;
        //格式化日志的日期和时间
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);

        //更新缓冲区写入位置 
        buff_.HasWritten(n);
        //更新缓冲区写入位置
        AppendLogLevelTitle_(level);

        va_start(vaList, format);
        //格式化后的日志消息添加到缓冲区
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);
        //更新缓冲区写入位置
        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        //如果是异步写入且阻塞队列未满，将日志字符串推送到阻塞队列
        if(isAsync_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.RetrieveAllToStr());
        }
        //否则直接将日志字符串写入文件。 
        else {
            fputs(buff_.Peek(), fp_);
        }
        buff_.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle_(int level) {
    switch(level) {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

void Log::flush() {
    if(isAsync_) { 
        deque_->flush(); 
    }
    fflush(fp_);
}

//异步写
/*
    从阻塞队列中获取日志字符串，并在临界区内写入文件
*/
void Log::AsyncWrite_() {
    string str = "";
    //从阻塞队列 deque_ 中弹出日志字符串，阻塞操作，如果队列为空，线程会等待，直到有数据可用
    while(deque_->pop(str)) {
        //加锁保证线程安全
        lock_guard<mutex> locker(mtx_);
        //将日志字符串写入文件
        fputs(str.c_str(), fp_);
    }
}

Log* Log::Instance() {
    static Log inst;
    return &inst;
}

//调用单例对象的AsyncWrite_() 启动异步写入日志的线程
void Log::FlushLogThread() {
    //获取 Log 类的单例对象，然后调用单例对象的 AsyncWrite_ 函数
    Log::Instance()->AsyncWrite_();
}