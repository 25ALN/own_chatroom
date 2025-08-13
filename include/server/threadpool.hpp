#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <queue>
#include <utility>


class threadpool{
public:
    explicit threadpool(size_t threadnum=std::thread::hardware_concurrency()):stop(false){
    //std::thread::hardware_concurrency()可返回系统上可以执行的最大线程数，通常是CPU核心数
        for(int i=0;i<threadnum;i++){
            s.emplace_back([this](){ //lambda的构建捕获this后就可调用这个类中的各个成员
                while(true){ //除非输入相应的信号否则一直处理任务队列
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->qmutex);
                        this->qc.wait(lock,[this](){
                            return this->stop==true||!this->tasks.empty();
                        });
                        if(this->stop&&this->tasks.empty()){
                            return;
                        }
                        task=std::move(this->tasks.front()); //加上move后效率更高，移动赋值，避免不必要的拷贝
                        this->tasks.pop();
                    }
                    //将锁规定在一个作用域内，离开后会自动解锁，这样可以防止锁住执行时间长的任务
                    task(); //开始执行
                }
            });
        }
    }

    //template<typename F,typename hd=std::function<void(return_type)>>
    template<typename F,typename... Args>
    //auto queuetasks(F&&f,std::vector<return_type> q){ //定义了一个参数获取回调的东西
    auto queuetasks(F&& f, Args&&... args)-> std::future<typename std::result_of<F(Args...)>::type> {
        //using return_type=typename std::result_of<F(Args...)>::type;
        using return_type = decltype(f(args...));
        auto task=std::make_shared<std::packaged_task<return_type()>>([=](){  //对任务进行封装
            return f(std::forward<Args>(args)...);
        }); 
        std::future<return_type> r=task->get_future();
        {
            std::unique_lock<std::mutex> lock(qmutex);
            tasks.emplace([task](){
            std::thread::id threadid=std::this_thread::get_id();
            (*task)();
            });
        }
        qc.notify_one();
        return r;
    }
    ~threadpool(){
        {
            std::unique_lock<std::mutex> lock(qmutex);
            stop=true;  
        }
        qc.notify_all(); //唤醒所有线程，准备清理掉
        for(std::thread &deal:s){
            deal.join();
        }
    }

private:
    bool stop;//用来控制线程池是否停止运行的标志。初始化为 false 表示线程池默认情况下是启动的。
    std::vector<std::thread> s;
    std::mutex qmutex;
    std::condition_variable qc;
    std::queue<std::function<void()> > tasks;
};