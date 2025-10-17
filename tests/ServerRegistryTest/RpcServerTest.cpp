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