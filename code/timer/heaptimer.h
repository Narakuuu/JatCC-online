#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>
#include "../log/log.h"

/* 定义类型和别名*/
typedef std::function<void()> TimeoutCallBack; //std::function<void()> 回调函数 不接受参数也不返回任何值
typedef std::chrono::high_resolution_clock Clock; // high_resolution_clock 测量时间的高精度时钟
typedef std::chrono::milliseconds MS; //milliseconds 时间毫秒数
typedef Clock::time_point TimeStamp; //time_point 时钟的时间点

struct TimerNode{
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode& t){
        return expires < t.expires;
    }
};

class HeapTimer{
public:
    HeapTimer() { heap_.reserve(64); }

    ~HeapTimer() { clear(); }

    void adjust(int id, int newExpires);

    void add(int id, int timeOut, const TimeoutCallBack& cb);

    void doWork(int id);

    void clear();

    void tick();

    void pop();

    int GetNextTick();

private:
    void del_(size_t i);

    void siftup_(size_t i);

    bool siftdown_(size_t index, size_t n);

    void SwapNode_(size_t i, size_t j );

    //存储定时器节点的容器，使用最小堆来维护节点的到期时间。
    std::vector<TimerNode> heap_;
    //ref_ 是哈希表，用于记录定时器节点的在堆中的索引信息, int 是定时器的唯一标识符id, size_t 记录节点在heap_堆中的索引位置
    std::unordered_map<int, size_t> ref_;
};





#endif