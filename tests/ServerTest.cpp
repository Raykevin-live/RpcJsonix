#include "../source/Message.hpp"
#include "../source/Net.hpp"
#include "../source/Dispatcher.hpp"

using namespace base;

void OnRpcMessage(const BaseConnection::Ptr& conn, RpcRequest::Ptr& msg)
{
    std::string body = msg->Serialize();
    std::cout<<body<<std::endl;
    auto rpc_req = MessageFactory::Create<RpcResponse>();
    rpc_req->SetId("123");
    rpc_req->SetMessType(MessType::RESPONSE_RPC);
    rpc_req->SetRcode(ResCode::RCODE_OK);
    rpc_req->SetResult(33);
}

void OnTopicMessage(const BaseConnection::Ptr& conn, TopicRequest::Ptr& msg)
{
    std::string body = msg->Serialize();
    std::cout<<body<<std::endl;
    auto rpc_req = MessageFactory::Create<RpcResponse>();
    rpc_req->SetId("123");
    rpc_req->SetMessType(MessType::RESPONSE_RPC);
    rpc_req->SetRcode(ResCode::RCODE_OK);
    rpc_req->SetResult(33);
}

int main(){
    auto dispatcher = std::make_shared<Dispatcher>();
    dispatcher->RegisterHandler<RpcRequest>(MessType::REQUEST_RPC, OnRpcMessage); //注册映射关系
    dispatcher->RegisterHandler<TopicRequest>(MessType::REQUEST_TOPIC, OnTopicMessage); //注册映射关系

    auto server = base::ServerFactory::Create(9090);
    auto message_callback = std::bind(&Dispatcher::OnMessage, dispatcher.get(),
                            std::placeholders::_1, std::placeholders::_2);
    server->SetMessageCallBack(message_callback);
    
    server->Start();
    return 0;
}