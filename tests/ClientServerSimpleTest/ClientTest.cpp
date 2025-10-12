#include "../source/common/Dispatcher.hpp"
#include "../source/client/Requestor.hpp"
#include "../source/client/RpcCaller.hpp"
#include <thread>

using namespace base;
using namespace client;
void OnRpcResponse(const BaseConnection::Ptr& conn, RpcResponse::Ptr& msg)
{
    std::cout<<"收到了Rpc响应"<<std::endl;
    std::string body = msg->Serialize();
    std::cout<<body<<std::endl;
}
void OnTopicResponse(const BaseConnection::Ptr& conn, TopicResponse::Ptr& msg)
{
    std::cout<<"收到了Topic响应"<<std::endl;
    std::string body = msg->Serialize();
    std::cout<<body<<std::endl;
}
void callback(const Json::Value& result){
    LOG_INFO("callback result: {}", result.asInt());
}
int main(){
    auto requestor = std::make_shared<Requestor>();
    auto caller = std::make_shared<RpcCaller>(requestor);

    auto dispatcher = std::make_shared<Dispatcher>();
    auto cb = std::bind(&Requestor::OnResponse, requestor.get(), 
                        std::placeholders::_1, std::placeholders::_2);
    dispatcher->RegisterHandler<BaseMessage>(MessType::RESPONSE_RPC, cb); //注册映射关系

    auto client = base::ClientFactory::Create("127.0.0.1", 9090);
    auto message_callback = std::bind(&Dispatcher::OnMessage, dispatcher.get(), 
                            std::placeholders::_1, std::placeholders::_2);
    client->SetMessageCallBack(message_callback);
    client->Connect();
    
    Json::Value param, result;
    param["num1"] = 11;
    param["num2"] = 22;
    auto conn = client->GetConnection();
    auto ret = caller->Call(conn, "Add", param, result);
    if(ret != false){
        LOG_INFO("result: {}", result.asInt());
    }

    param["num1"] = 33;
    param["num2"] = 44;
    RpcCaller::JsonAsyncResponse res_future;
    ret = caller->Call(conn, "Add", param, res_future);
    if(ret != false){
        result = res_future.get(); //异步-这里才获取结果
        LOG_INFO("result: {}", result.asInt());
    }

    param["num1"] = 44;
    param["num2"] = 55;
    ret = caller->Call(conn, "Add", param, callback);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    client->Shutdown();
    return 0;
}

