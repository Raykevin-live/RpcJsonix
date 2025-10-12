#include "../common/Dispatcher.hpp"
#include "RpcRouter.hpp"
#include "RpcRegistry.hpp"
#include "../client/RpcClient.hpp"

using namespace base;
namespace server{
/**
 * @brief 注册中心服务端：只需要针对服务注册与发现请求进行处理即可
 * 
 */
class RegistryServer{
public:
    using Ptr = std::shared_ptr<RegistryServer>;
    RegistryServer(int16_t port)
        :_pd_manager(std::make_shared<PDManager>())
        ,_dispatcher(std::make_shared<Dispatcher>())
        {
             auto service_cb = std::bind(&PDManager::OnServiceRequest, _pd_manager.get(), 
                        std::placeholders::_1, std::placeholders::_2);
            _dispatcher->RegisterHandler<ServiceRequest>(MessType::REQUEST_SERVICE, service_cb); //注册映射关系
            
            _server = base::ServerFactory::Create(port);

            auto message_callback = std::bind(&Dispatcher::OnMessage, _dispatcher.get(),
                                    std::placeholders::_1, std::placeholders::_2);
            _server->SetMessageCallBack(message_callback);

            auto close_callback = std::bind(&RegistryServer::__OnConnShutDown, this, std::placeholders::_1);
            _server->SetCloseCallBack(close_callback);
        }
        void Start(){
            _server->Start();
        }
private:
    void __OnConnShutDown(const BaseConnection::Ptr& conn){
        _pd_manager->OnConnShutdown(conn);
    }
private:
    PDManager::Ptr _pd_manager;
    Dispatcher::Ptr _dispatcher;
    BaseServer::Ptr _server;
};

class RpcServer{
public:
    using Ptr = std::shared_ptr<RpcServer>;
    // rpc_server 端有两套地址信息，
    // 1. rpc服务提供端地址信息 -- 必须是rpc服务端对外访问地址（云服务器 -- 监听地址和访问地址不同）
    // 2. 注册中心服务端地址信息 -- 启用服务注册后，连接注册中心进行服务注册
    RpcServer(const Address& access_addr, 
            bool enableRegistry = false,
            const Address& registry_server_addr = Address())
        :_access_addr(access_addr)
        ,_enable_registry(enableRegistry)
        ,_router(std::make_shared<RpcRouter>())
        ,_dispatcher(std::make_shared<Dispatcher>())
        {
            if(enableRegistry == true){
                _client_registry = std::make_shared<client::RegistryClient>(
                    registry_server_addr.first, registry_server_addr.second);
            }
            // 当前成员server 是一个rpc_server，用于提供rpc服务的
            auto rpc_cb = std::bind(&RpcRouter::OnRpcRequest, _router.get(), 
                        std::placeholders::_1, std::placeholders::_2);
            _dispatcher->RegisterHandler<RpcRequest>(MessType::REQUEST_RPC, rpc_cb); //注册映射关系

            _server = base::ServerFactory::Create(access_addr.second);

            auto message_callback = std::bind(&Dispatcher::OnMessage, _dispatcher.get(),
                                    std::placeholders::_1, std::placeholders::_2);
            _server->SetMessageCallBack(message_callback);
        }
    void RegistryMethod(const ServiceDiscribe::Ptr& service){
        if(_enable_registry){
            _client_registry->RegistryMethod(service->MethodName(), _access_addr);
        }
        _router->RegisterMethod(service);
    }
    void Start(){
        _server->Start();
    }
private:
    Address _access_addr;
    bool _enable_registry;    
    RpcRouter::Ptr _router;
    Dispatcher::Ptr _dispatcher;
    BaseServer::Ptr _server;
    client::RegistryClient::Ptr _client_registry;
};
}