#include<iostream>
#include<future>
#include <thread>

int Add(int num1, int num2)
{
    std::cout<<"into Add function"<<std::endl;
    return num1 + num2;
}

int main()
{
    // std::launch::deferred -> 同步策略，需要结果的时候再去执行
    // std::launch::launch -> 内部创建一个线程执行函数，函数运行结束通过future获取
    std::future<int> res = std::async(std::launch::deferred, Add, 11, 22); // 启动一个异步非阻塞调用
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout<<"--------------------------\n";
    std::cout<<res.get()<<std::endl; //获取结果，如果异步任务没有执行完就阻塞

    return 0;
}