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