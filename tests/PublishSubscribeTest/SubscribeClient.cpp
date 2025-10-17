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