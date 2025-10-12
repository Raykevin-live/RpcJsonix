#include "../../source/client/RpcClient.hpp"
#include "../../source/common/Logging.hpp"

using namespace base;
using namespace client;

void callback(const Json::Value& result){
    LOG_INFO("callback result: {}", result.asInt());
}
int main(){
    RpcClient client(false, "127.0.0.1", 9090);

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

