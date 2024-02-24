#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <assert.h>

class ThreadPool{
public:
    //构造函数，用于初始化线程池. pool_指向一个结构体Pool实例
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()){
        assert(threadCount > 0);
        //使用循环创建指定数量的线程
        for( size_t i = 0; i < threadCount; i++){
            //lambda函数 每个线程都执行以下逻辑
            
            std::thread([pool = pool_]{
                std::unique_lock<std::mutex> locker(pool->mtx);
                while(true){
                    if( !pool->tasks.empty()){
                        //如果任务队列非空，则取出队首任务并执行
                        //转移任务所有权，避免不必要复制
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        //释放互斥锁，允许其他线程访问线程池以便其他线程有机会添加新任务。
                        locker.unlock();
                        //执行任务
                        task();
                        //再次上锁，保证 判空、转移所有权以及出队的原子性操作
                        locker.lock();
                    }
                    else if(pool->isClosed) break;
                    /*
                        wait 函数传入互斥锁，在等待过程中释放锁，而等待结束（条件满足或被通知时）
                        重新锁住互斥锁(locker的析构函数)，确保等待过程中线程安全地释放和重新获得互斥锁。
                    */
                    else pool->cond.wait(locker);
                }
            }).detach();//将线程设置为后台线程，主线程无需等待其完成
        }
    }
    ThreadPool() = default;

    ThreadPool( ThreadPool&& ) = default;

    ~ThreadPool(){
        //检查 pool_ 指针是否为空
        if(static_cast<bool>(pool_)){
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->isClosed = true;
        }
        pool_->cond.notify_all();
    }

    template<class F>
    void AddTask(F&& task){
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            //使用 std::forward 完美转发
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }
    
    
private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClosed;
        //存储函数对象的队列，用于保存待执行的任务
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;

};


#endif