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
    // 1. 封装任务
    std::packaged_task<int(int, int)> task(Add);

    // 2. 获取任务包关联的future对象
    std::future<int> res = task.get_future();
    // 3. 执行任务
    task(11, 12);

    // 4.获取结果
    std::cout<<res.get()<<std::endl;
    return 0;
}