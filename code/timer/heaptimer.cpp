#include "heaptimer.h"

//上虑操作，将堆中索引为 i 的节点上浮到正确的位置，以满足堆的性质
void HeapTimer::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    //计算父节点的索引
    size_t j = (i - 1) / 2;
    while(j >= 0) {
        //检查父节点的值是否小于节点 i 的值，如果是，则退出循环
        if(heap_[j] < heap_[i]) { break; }
        //否则交换位置
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

//交换索引为 i j 的两个节点
void HeapTimer::SwapNode_(size_t i, size_t j) {
    //确保索引 i 和 j 在有效范围内
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    //swap 函数交换堆中索引为 i 和 j 的两个节点，swap 交换两个TimerNode结构体
    std::swap(heap_[i], heap_[j]);
    //更新哈希表 ref_ 的映射关系
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
} 


//下虑操作
bool HeapTimer::siftdown_(size_t index, size_t n) {
    //确保输入的索引 index 和堆的大小 n 在有效范围内
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    //计算左子节点的索引，j 表示左子节点的索引
    size_t j = i * 2 + 1;
    while(j < n) {
        //检查右子节点是否存在且小于左子节点，如果是，则更新 j 为右子节点的索引
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        //如果当前节点值小于左右孩子中较小的孩子的值，说明满足最小堆
        if(heap_[i] < heap_[j]) break;
        //否则交换位置
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}


//添加节点
void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);
    size_t i;
    if(ref_.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftup_(i);
    } 
    else {
        /* 已有结点：调整堆 */
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if(!siftdown_(i, heap_.size())) {
            siftup_(i);
        }
    }
}


void HeapTimer::doWork(int id) {
    /* 删除指定id结点，并触发回调函数 */
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    //找到 id 对应的堆中索引位置
    size_t i = ref_[id];
    //通过索引找到节点
    TimerNode node = heap_[i];
    //触发回调函数
    node.cb();
    //删除节点
    del_(i);
}

void HeapTimer::del_(size_t index) {
    /* 删除指定位置的结点 */
    //确保堆不为空，且要删除的节点索引 index 在有效范围内
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = heap_.size() - 1;
    //确保要删除的节点索引 i 小于等于最后一个节点的索引 n
    assert(i <= n);
    if(i < n) {
        //交换要删除的节点与最后一个节点交换位置
        SwapNode_(i, n);
        //如果通过下虑操作无法维护堆的性质则尝试通过上虑操作来维护堆的性质。
        if(!siftdown_(i, n)) {
            siftup_(i);
        }
    }
    /* 队尾元素删除 */
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);;
    siftdown_(ref_[id], heap_.size());
}

//清除超时的定时器节点
void HeapTimer::tick() {
    if(heap_.empty()) {
        return;
    }
    while(!heap_.empty()) {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

//获得距离下一个定时器到期的时间
int HeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}