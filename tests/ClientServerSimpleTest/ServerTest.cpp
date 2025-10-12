#include "../source/common/Dispatcher.hpp"
#include "../source/server/RpcRouter.hpp"

using namespace base;
using namespace server;

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

void Add(const Json::Value& req, Json::Value& rsp){
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    rsp = num1 + num2;
}
int main(){
    // 初始化分发器
    auto dispatcher = std::make_shared<Dispatcher>();
    // 初始化router
    auto router = std::make_shared<RpcRouter>();
    // 初始化服务构造器
    std::unique_ptr<ServiceDiscribeFactory> server_factory(new ServiceDiscribeFactory());
    // 服务构造器设置
    server_factory->SetMethodName("Add");
    server_factory->SetParamsDesc("num1", ValueType::INTERGRAL);
    server_factory->SetParamsDesc("num2", ValueType::INTERGRAL);
    server_factory->SetReturnType(ValueType::INTERGRAL);
    server_factory->SetCallback(Add);
    // 注册服务到router
    router->RegisterMethod(server_factory->Build());
    
    // 绑定rpc回调方法
    auto cb = std::bind(&RpcRouter::OnRpcRequest, router.get(), 
                        std::placeholders::_1, std::placeholders::_2);
    dispatcher->RegisterHandler<RpcRequest>(MessType::REQUEST_RPC, cb); //注册映射关系
    

    auto server = base::ServerFactory::Create(9090);
    auto message_callback = std::bind(&Dispatcher::OnMessage, dispatcher.get(),
                            std::placeholders::_1, std::placeholders::_2);
    server->SetMessageCallBack(message_callback);
    
    server->Start();
    return 0;
}