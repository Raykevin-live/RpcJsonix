#include "../common/Dispatcher.hpp"
#include "Requestor.hpp"
#include "RpcCaller.hpp"
#include "RpcRegistry.hpp"
#include "RpcTopic.hpp"

/**
 * @brief 本文件主要进行客户端实现，对客户端部分模块进行集成
 * 
 */
using namespace base;
namespace client{
class RegistryClient{
public:
    using Ptr = std::shared_ptr<RegistryClient>;
    // 构造函数传入注册中心的地址信息，用于连接注册中心
    RegistryClient(const std::string& ip, int16_t port)
        :_requestor(std::make_shared<Requestor>())
        ,_provider(std::make_shared<Provider>(_requestor)) //这里通过已有requestor构造provider，进行管理
        ,_dispatcher(std::make_shared<Dispatcher>())
        {
            auto rsp_cb = std::bind(&Requestor::OnResponse, _requestor.get(), 
                    std::placeholders::_1, std::placeholders::_2);
            _dispatcher->RegisterHandler<BaseMessage>(MessType::RESPONSE_SERVICE, rsp_cb);
            
            auto message_callback = std::bind(&Dispatcher::OnMessage, _dispatcher.get(), 
                    std::placeholders::_1, std::placeholders::_2);
            _client = ClientFactory::Create(ip, port);
            _client->SetMessageCallBack(message_callback);
            _client->Connect();
        }
    // 向外提供的服务注册接口
    bool RegistryMethod(const std::string& method, const Address& host){
        return _provider->RegistryMethod(_client->GetConnection(), method, host);
    }
private:
    Requestor::Ptr _requestor;
    Provider::Ptr _provider; 
    Dispatcher::Ptr _dispatcher;
    BaseClient::Ptr _client;
};

class DiscoveryClient{
public:
    using Ptr = std::shared_ptr<DiscoveryClient>;
    // 构造函数传入发现中心的地址信息，用于连接注册中心
    DiscoveryClient(const std::string& ip, int16_t port, const Discoverer::OfflineCallback& callback)
    :_requestor(std::make_shared<Requestor>())
    ,_discoverer(std::make_shared<Discoverer>(_requestor, callback))
    ,_dispatcher(std::make_shared<Dispatcher>()){
        LOG_INFO("构造发现客户端...");
        auto rsp_cb = std::bind(&Requestor::OnResponse, _requestor.get(), 
                    std::placeholders::_1, std::placeholders::_2);
        _dispatcher->RegisterHandler<BaseMessage>(MessType::RESPONSE_SERVICE, rsp_cb);
        auto req_cb = std::bind(&Discoverer::OnserviceRequest, _discoverer.get(), 
                    std::placeholders::_1, std::placeholders::_2);
        _dispatcher->RegisterHandler<ServiceRequest>(MessType::REQUEST_SERVICE, req_cb);

        auto message_callback = std::bind(&Dispatcher::OnMessage, _dispatcher.get(), 
                std::placeholders::_1, std::placeholders::_2);
        _client = ClientFactory::Create(ip, port);
        _client->SetMessageCallBack(message_callback);
        _client->Connect();
    }
    /**
     * @brief 向外提供服务发现接口
     * 
     * @param method 
     * @param host 输出型参数，返回可以支持该服务的一个主机
     * @return true 
     * @return false 
     */
    bool ServiceDiscovery(const std::string& method, Address& host){
        return _discoverer->ServiceDiscovery(_client->GetConnection(), method, host);
    }
private:
    Requestor::Ptr _requestor;
    Discoverer::Ptr _discoverer;
    Dispatcher::Ptr _dispatcher;
    BaseClient::Ptr _client;
};

class RpcClient{
public:
    using Ptr = std::shared_ptr<RpcClient>;
    /**
         * @brief Construct a new Rpc Client object
         * 
         * @param enableDiscovery 是否启用服务发现功能
         * @param ip 
         * @param port 
         * @details 如果启用服务发现，则传入注册中心地址，否则传入服务提供者地址
         */
    RpcClient(bool enableDiscovery, const std::string& ip, int16_t port)
        :_enable_discovery(enableDiscovery)
        ,_requestor(std::make_shared<Requestor>())
        ,_dispatcher(std::make_shared<Dispatcher>())
        ,_caller(std::make_shared<RpcCaller>(_requestor))
        {
            // 针对Rpc调用
            auto rsp_cb = std::bind(&Requestor::OnResponse, _requestor.get(), 
                    std::placeholders::_1, std::placeholders::_2);
            _dispatcher->RegisterHandler<BaseMessage>(MessType::RESPONSE_RPC, rsp_cb);

            // 如果启用了服务发现，地址信息是注册中心地址信息，是服务发现客户端需要连接的地址，
            // 通过地址信息实例化_discovery_client
            // 如果没有启用服务发现，地址信息是服务提供者的地址信息，则直接实例化rpc_client
            if(_enable_discovery == true){
                auto offline_callback = std::bind(&RpcClient::__DelClient, this, std::placeholders::_1);
                _discovery_client = std::make_shared<DiscoveryClient>(ip, port, offline_callback);
            }
            else{
                auto message_callback = std::bind(&Dispatcher::OnMessage, _dispatcher.get(), 
                    std::placeholders::_1, std::placeholders::_2);
                _rpc_client = ClientFactory::Create(ip, port);
                _rpc_client->SetMessageCallBack(message_callback);
                _rpc_client->Connect();
            }
        }
    // 同步响应
    bool Call(const std::string& method, const Json::Value& params, Json::Value& result){
        // 获取服务提供者： a. 没启用服务发现，去列表查找；b. 启用服务发现，使用固定提供者
        auto client = _GetUsefulClient(method);
        if(client.get() == nullptr){
            return false;
        }
        // 3.通过客户端连接，发送rpc请求
        return _caller->Call(client->GetConnection(), method, params, result);
    }
    // 异步响应
    bool Call(const std::string& method, const Json::Value& params, RpcCaller::JsonAsyncResponse& result){
        auto client = _GetUsefulClient(method);
        if(client.get() == nullptr){
            return false;
        }
        // 3.通过客户端连接，发送rpc请求
        return _caller->Call(client->GetConnection(), method, params, result);
    }
    bool Call(const std::string& method, const Json::Value& params, const RpcCaller::JsonResponseCallback& callback){
        auto client = _GetUsefulClient(method);
        if(client.get() == nullptr){
            return false;
        }
        // 3.通过客户端连接，发送rpc请求
        return _caller->Call(client->GetConnection(), method, params, callback);
    }
private:
    BaseClient::Ptr __NewClient(const Address& host){
        auto message_callback = std::bind(&Dispatcher::OnMessage, _dispatcher.get(), 
                    std::placeholders::_1, std::placeholders::_2);
        auto client = ClientFactory::Create(host.first, host.second);
        client->SetMessageCallBack(message_callback);
        client->Connect();
        // 管理起来
        __PutClient(host, client);
        return client;
    }
    BaseClient::Ptr _GetUsefulClient(const std::string& method){
        BaseClient::Ptr client;
        if(_enable_discovery){
            Address host;
            bool ret = _discovery_client->ServiceDiscovery(method, host);
            if(ret == false){
                LOG_ERROR("当前 {} 服务，没有找到服务提供者！", method);
                return BaseClient::Ptr();
            }
            // 2.查看服务提供者是否已有示例化客户端，有则直接使用，没有则创建
            client = __GetClient(host);
            if(client.get() == nullptr){
                client = __NewClient(host);
            }
        }
        else{
            client = _rpc_client;
        }
        return client;
    }
    BaseClient::Ptr __GetClient(const Address& host){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _rpc_clients.find(host);
        if(it == _rpc_clients.end()){
            return BaseClient::Ptr();
        }
        return it->second;
    }
    void __PutClient(const Address& host, const BaseClient::Ptr& client){
        std::unique_lock<std::mutex> lock(_mutex);
        _rpc_clients.emplace(host, client);
    }
    /**
         * @brief 回调给上层删除，防止泄漏
         * 
         * @param host 
         */
    void __DelClient(const Address& host){
        std::unique_lock<std::mutex> lock(_mutex);
        _rpc_clients.erase(host);
    }
private:
    /**
     * @brief 定义仿函数作为自定义类型红黑树结构的key值
     * 
     */
    struct AddressHash
    {
        size_t operator()(const Address& host) const noexcept{
            std::string addr = host.first + std::to_string(host.second);
            return std::hash<std::string>{}(addr);
        }
    };
private:
    bool _enable_discovery;
    Requestor::Ptr _requestor;
    DiscoveryClient::Ptr _discovery_client; ///< 可以进行服务发现
    RpcCaller::Ptr _caller;
    Dispatcher::Ptr _dispatcher;
    BaseClient::Ptr _rpc_client; //用于未启用服务发现的客户端
    std::mutex _mutex;
    // hash<host, client>
    std::unordered_map<Address, BaseClient::Ptr, AddressHash> _rpc_clients; ///< 用于启用服务发现后的连接池
};
class TopicClient{
public:
    TopicClient(const std::string &ip, int16_t port)
        :_requestor(std::make_shared<Requestor>())
        ,_dispatcher(std::make_shared<Dispatcher>())
        ,_topic_manager(std::make_shared<TopicManager>(_requestor)) {
            auto rsp_cb = std::bind(&Requestor::OnResponse, _requestor.get(), 
                                    std::placeholders::_1, std::placeholders::_2);
            _dispatcher->RegisterHandler<BaseMessage>(MessType::RESPONSE_TOPIC, rsp_cb);
            auto msg_cb = std::bind(&TopicManager::OnPublish, _topic_manager.get(), 
                                    std::placeholders::_1, std::placeholders::_2);
            _dispatcher->RegisterHandler<TopicRequest>(MessType::REQUEST_TOPIC, msg_cb);

            auto message_cb = std::bind(&Dispatcher::OnMessage, _dispatcher.get(), 
                                        std::placeholders::_1, std::placeholders::_2);
            _rpc_client = ClientFactory::Create(ip, port);
            _rpc_client->SetMessageCallBack(message_cb);
            _rpc_client->Connect();
        }
    bool Create(const std::string& key){
        return _topic_manager->Create(_rpc_client->GetConnection(), key);
    }
    bool Remove(const std::string& key){
        return _topic_manager->Remove(_rpc_client->GetConnection(), key);
    }
    bool Subscribe(const std::string& key, const TopicManager::SubCallback& cb){
       return _topic_manager->Subscribe(_rpc_client->GetConnection(), key, cb);
    }
    bool Cancel(const std::string& key){
        return _topic_manager->Cancel(_rpc_client->GetConnection(), key);
    }
    bool Publish(const std::string& key, const std::string& msg){
        return _topic_manager->Publish(_rpc_client->GetConnection(), key, msg);
    }
    void ShutDown(){
        _rpc_client->Shutdown();
    }
private:
    Requestor::Ptr _requestor;
    TopicManager::Ptr _topic_manager;
    Dispatcher::Ptr _dispatcher;
    BaseClient::Ptr _rpc_client; //用于未启用服务发现的客户端
};
}