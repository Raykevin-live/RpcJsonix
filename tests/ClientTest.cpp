#include "../source/Message.hpp"
#include "../source/Net.hpp"
#include "../source/Dispatcher.hpp"
#include <thread>

using namespace base;

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
int main(){
    auto dispatcher = std::make_shared<Dispatcher>();
    dispatcher->RegisterHandler<RpcResponse>(MessType::RESPONSE_RPC, OnRpcResponse); //注册映射关系
    dispatcher->RegisterHandler<TopicResponse>(MessType::RESPONSE_TOPIC, OnTopicResponse); //注册映射关系

    auto client = base::ClientFactory::Create("127.0.0.1", 9090);
    auto message_callback = std::bind(&Dispatcher::OnMessage, dispatcher.get(), 
                            std::placeholders::_1, std::placeholders::_2);
    client->SetMessageCallBack(message_callback);
    client->Connect();
    
    auto rpc_req = MessageFactory::Create<RpcRequest>();
    rpc_req->SetId("1234");
    rpc_req->SetMessType(MessType::REQUEST_RPC);
    rpc_req->SetMethod("Add");
    Json::Value param;
    param["num1"] = 11;
    param["num2"] = 22;
    rpc_req->SetParams(param);

    client->Send(rpc_req);

    auto topic_req = MessageFactory::Create<TopicRequest>();
    topic_req->SetId("4321");
    topic_req->SetMessType(MessType::REQUEST_TOPIC);
    topic_req->SetTopicMeassage("hello");
    topic_req->SetTopicKey("Say");
    topic_req->SetTopicOperType(TopicOperType::TOPIC_SUBSCRIBE);
    client->Send(topic_req);


    std::this_thread::sleep_for(std::chrono::seconds(10));
    client->Shutdown();
    return 0;
}

