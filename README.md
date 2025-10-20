# RpcJsonix
![Static Badge](https://img.shields.io/badge/build-passing-brightgreen%26nbsp%3B?style=plastic)&nbsp;![Static Badge](https://img.shields.io/badge/license-GunPublic3.0-green?style=plastic)&nbsp;![Static Badge](https://img.shields.io/badge/release-v1.1.0-blue?style=plastic)


## 介绍
&nbsp;&nbsp;&nbsp;&nbsp;本项目为基于Json序列化以及C++网络库Muduo库的轻量级Rpc框架
## 目录
[TOC]
## 文件目录
- `demo/`: C++异步编程、Json序列化、Muduo库的demo样例
- `images/`: 项目中包含的图片信息
- `source/`: 项目源文件目录
  - `client/`: 客户端文件目录
  - `common/`: 公共模块文件
  - `server/`：服务端模块
- `tests`: 测试文件目录
- `thirds`: 第三方库目录
## 环境配置
- 平台环境: `Ubuntu 22.04.4 LTS`
- **C++环境**: C++17及以上
- Jsoncpp库: `amd64 1.9.5-3`
- muduo库: `/`
- gcc/g++: gcc11/g++11及以上
- Cmake: 3.17及以上
## 技术栈
- Json序列化、网络通信(muduo)、Rpc、、TCP/IP、分布式家都
## 功能支持
- **服务调用**
  - 同步调用
  - 异步callback调用
  - 异步future调用
- **负载均衡**
- **服务注册与发现**
- **服务上/下线通知**
- **发布订阅**
## 项目架构
![项目架构图](/images/ArchitectureDiagram.png)
## 用法示例
### 1.服务注册与发现:
服务注册服务端
```c++
#include "../../source/server/RpcServer.hpp"
#include "../../source/common/Logging.hpp"

using namespace base;
using namespace server;

int main(){
    RegistryServer reg_server(8080);
    reg_server.Start();

    return 0;
}
```
Rpc服务端
```c++
#include "../../source/server/RpcServer.hpp"
#include "../../source/common/Logging.hpp"

using namespace base;
using namespace server;

void Add(const Json::Value& req, Json::Value& rsp){
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    rsp = num1 + num2;
}

int main(){
    // 初始化服务构造器
    std::unique_ptr<ServiceDiscribeFactory> server_factory(new ServiceDiscribeFactory());
    // 服务构造器设置
    server_factory->SetMethodName("Add");
    server_factory->SetParamsDesc("num1", ValueType::INTERGRAL);
    server_factory->SetParamsDesc("num2", ValueType::INTERGRAL);
    server_factory->SetReturnType(ValueType::INTERGRAL);
    server_factory->SetCallback(Add);
    
    RpcServer server({"127.0.0.1", 9090}, true, {"127.0.0.1", 8080});
    server.RegistryMethod(server_factory->Build());
    server.Start();

    return 0;
}
```
Rpc客户端
```c++
#include "../../source/client/RpcClient.hpp"
#include "../../source/common/Logging.hpp"

using namespace base;
using namespace client;

void callback(const Json::Value& result){
    LOG_INFO("callback result: {}", result.asInt());
}
int main(){
    RpcClient client(true, "127.0.0.1", 8080);

    Json::Value param, result;
    param["num1"] = 11;
    param["num2"] = 22;
    auto ret = client.Call("Add", param, result);
    if(ret != false){
        LOG_INFO("result: {}", result.asInt());
    }

    param["num1"] = 33;
    param["num2"] = 44;
    RpcCaller::JsonAsyncResponse res_future;
    ret = client.Call("Add", param, res_future);
    if(ret != false){
        result = res_future.get(); //异步-这里才获取结果
        LOG_INFO("result: {}", result.asInt());
    }

    param["num1"] = 44;
    param["num2"] = 55;
    ret = client.Call("Add", param, callback);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
```
### 2.主题发布与订阅
主题服务端
```c++
#include "../../source/server/RpcServer.hpp"

int main(){
    auto server = std::make_shared<server::TopicServer>(7070);
    server->Start();

    return 0;
}
```
主题发布客户端
```c++
#include "../../source/client/RpcClient.hpp"

using namespace client;
int main(){
    // 1. 示例化客户端对象
    auto client = std::make_shared<TopicClient>("127.0.0.1", 7070);
    // 2. 创建主题
    bool ret = client->Create("hello");
    if(ret == false){
        LOG_ERROR("创建主题失败");
        return -1;
    }
    LOG_INFO("创建主题成功");
    // 3. 向主题发布消息
    for(int i = 0; i<5 ; ++i){
        client->Publish("hello", "Hello world" + std::to_string(i+1));
    }
    
    client->ShutDown();
    return 0;
}
```
主题订阅客户端
```c++
#include "../../source/client/RpcClient.hpp"

void callback(const std::string& key, const std::string& msg){
    LOG_INFO("{} 主题收到推送过来的消息: {}", key, msg);
}
int main(){
    // 1. 示例化对象
    auto client = std::make_shared<client::TopicClient>("127.0.0.1", 7070);
    // 2.创建主题
    bool ret = client->Create("hello");
    if(ret == false){
        LOG_ERROR("创建主题失败");
        return -1;
    }
    LOG_INFO("创建主题成功");
    // 3.订阅主题
    ret = client->Subscribe("hello", callback);
    // 4.等待->退出
    std::this_thread::sleep_for(std::chrono::seconds(20));
    client->ShutDown();
    return 0;
}
```