#include "../source/common/Message.hpp"

void TestRpcRequest(){
    auto rpc = base::MessageFactory::Create<base::RpcRequest>();
    Json::Value param;
    param["num1"] = 11;
    param["num2"] = 22;
    rpc->SetMethod("Add");
    rpc->SetParams(param);
    std::string str = rpc->Serialize();
    std::cout<<str<<std::endl;

    base::BaseMessage::Ptr ptr = base::MessageFactory::Create(common::MessType::REQUEST_RPC);
    bool ret = ptr->Unserialize(str);
    if(ret == false){
        return ;
    }
    ret = ptr->Check();
    if(ret == false){
        return ;
    }

    auto p_ret = std::dynamic_pointer_cast<base::RpcRequest>(ptr);
    std::cout<<p_ret->Method()<<std::endl;
    std::cout<<p_ret->Params()["num1"].asInt()<<std::endl;
    std::cout<<p_ret->Params()["num2"].asInt()<<std::endl;
}

void TestRpcResponse(){
    auto rpc = base::MessageFactory::Create<base::RpcResponse>();
    rpc->SetRcode(common::ResCode::RCODE_OK);
    rpc->SetResult(33);
    std::string str = rpc->Serialize();
    std::cout<<str<<std::endl;

    base::BaseMessage::Ptr ptr = base::MessageFactory::Create(common::MessType::RESPONSE_RPC);
    bool ret = ptr->Unserialize(str);
    if(ret == false){
        return ;
    }
    ret = ptr->Check();
    if(ret == false){
        return ;
    }

    auto p_ret = std::dynamic_pointer_cast<base::RpcResponse>(ptr);
    std::cout<<(int)p_ret->Rcode()<<std::endl;
    std::cout<<p_ret->Result().asInt()<<std::endl;
}

void TestTopicRequest(){
    auto topic = base::MessageFactory::Create<base::TopicRequest>();
    topic->SetTopicKey("news");
    topic->SetTopicOperType(common::TopicOperType::TOPIC_PUBLISH);
    topic->SetTopicMeassage("hello world");
    std::string str = topic->Serialize();
    std::cout<<str<<std::endl;

    auto ptr = base::MessageFactory::Create(common::MessType::REQUEST_TOPIC);
    bool ret = ptr->Unserialize(str);
    if(ret == false){
        return ;
    }
    ret = ptr->Check();
    if(ret == false){
        return ;
    }
    auto p_ret = std::dynamic_pointer_cast<base::TopicRequest>(ptr);
    std::cout<<p_ret->TopicKey()<<std::endl;
    std::cout<<(int)p_ret->TopicOperType()<<std::endl;
    std::cout<<p_ret->TopicMeassage()<<std::endl;
}

void TestServiceRequest(){
    auto service = base::MessageFactory::Create<base::ServiceRequest>();
    service->SetMethod("Add");
    service->SetServiceOperType(common::ServiceOperType::SERVICE_REGISRY);
    service->SetHostMeassage({"127.0.0.1", 9090});

    std::string str = service->Serialize();
    std::cout<<str<<std::endl;

    auto ptr = base::MessageFactory::Create(common::MessType::REQUEST_SERVICE);
    bool ret = ptr->Unserialize(str);
    if(ret == false){
        return ;
    }
    ret = ptr->Check();
    if(ret == false){
        return ;
    }
    auto p_ret = std::dynamic_pointer_cast<base::ServiceRequest>(ptr);
    std::cout<<p_ret->HostMeassage().first<<std::endl;
    std::cout<<p_ret->HostMeassage().second<<std::endl;
}

void TestServiceResponse(){
    auto service = base::MessageFactory::Create<base::ServiceResponse>();
    service->SetRcode(common::ResCode::RCODE_OK);
    service->SetId("Add");
    service->SetServiceOperType(common::ServiceOperType::SERVICE_REGISRY);
    service->SetHosts({{"127.0.0.1", 9090}, 
                        {"127.0.0.1", 8080}});
    std::string str = service->Serialize();
    std::cout<<str<<std::endl;

    base::BaseMessage::Ptr ptr = base::MessageFactory::Create(common::MessType::RESPONSE_SERVICE);
    bool ret = ptr->Unserialize(str);
    if(ret == false){
        return ;
    }
    ret = ptr->Check();
    if(ret == false){
        return ;
    }

    auto p_ret = std::dynamic_pointer_cast<base::ServiceResponse>(ptr);
    std::cout<<(int)p_ret->Rcode()<<std::endl;
    std::cout<<(int)p_ret->ServiceOperType()<<std::endl;
    std::cout<<p_ret->Method()<<std::endl;
    auto addrs = p_ret->Hosts();
    for(const auto& addr : addrs){
        std::cout<<addr.first<<" : "<<addr.second<<std::endl;
    }
}
int main()
{
    // TestRpcRequest();
    // TestTopicRequest();
    // TestServiceRequest();
    // TestRpcResponse();
    TestServiceResponse();
    return 0;
}