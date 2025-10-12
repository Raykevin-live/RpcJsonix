#pragma once
#include "../common/Net.hpp"
#include "../common/Message.hpp"
#include <set>
#include "../common/Uuid.hpp"

namespace server{
using namespace base;
/**
 * @brief 服务提供者管理类
 * @details 主机地址、提供服务的名称
 */
class ProviderManager{
public:
    using Ptr = std::shared_ptr<ProviderManager>;
    struct Provider{
        using Ptr = std::shared_ptr<Provider>;
        std::mutex p_mutex;
        BaseConnection::Ptr connect;
        Address host;
        std::vector<std::string> methods;

        Provider(const BaseConnection::Ptr& conn, const Address& host)
            :connect(conn), host(host)
        {}
        void AppendMethod(const std::string& method){
            std::unique_lock<std::mutex> lock(p_mutex);
            methods.emplace_back(method);
        }
    };
    // 当一个新的服务提供者进行服务注册时调用
    void AddProvider(const BaseConnection::Ptr& conn, const Address& host, const std::string& method){
        Provider::Ptr provider;
        // 查找连接所关联的服务提供者，找到则获取，找不到则创建，并建立关联
        {
            // 加作用域尽量减少锁的周期
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _conns.find(conn);
            if(it != _conns.end()){
                provider = it->second;
            }
            else{
                provider = std::make_shared<Provider>(conn, host);
                _conns.emplace(conn, provider);
            }
        // 注意：method方法所提供的主机要多出一个，新增_providers数据
            auto& providers = _providers[method];
            providers.insert(provider);
        }
        // 向服务对象中新增一个所能提供的服务名称
        provider->AppendMethod(method);
    }
    // 当一个服务提供者断开连接的时候，获取信息 -- 用于服务下线通知
    Provider::Ptr GetProvider(const BaseConnection::Ptr& conn){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _conns.find(conn);
        if(it != _conns.end()){
            return it->second;
        }
        return Provider::Ptr();
    }   

    /**
     * @brief 连接断开时进行删除关联信息
     * 
     * @param conn 
     */
    void DelProvider(const BaseConnection::Ptr& conn){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _conns.find(conn);
        if(it == _conns.end()){
            LOG_INFO("当前删除的服务非服务提供者");
            return;
        }
        for(auto& method : it->second->methods){
            auto& providers = _providers[method];
            providers.erase(it->second);
        }
        _conns.erase(conn);
    }
    std::vector<Address> GetMethodHosts(const std::string& method){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _providers.find(method);
        if(it == _providers.end()){
            return std::vector<Address>();
        }

        std::vector<Address> result;
        for(auto& provider : it->second){
            result.emplace_back(provider->host);
        }
        return result;
    }
private:
    std::mutex _mutex;
    std::unordered_map<std::string, std::set<Provider::Ptr>> _providers; ///< hash<method, vector<provider>>
    std::unordered_map<BaseConnection::Ptr, Provider::Ptr> _conns; ///< hash<conn, provider>
};

/**
 * @brief 服务端发现者管理类
 * @details 客户端连接、发现过的服务名称
 */
class DiscovererManager{
public:
    using Ptr = std::shared_ptr<DiscovererManager>;
    struct Discoverer
    {
        using Ptr = std::shared_ptr<Discoverer>;
        std::mutex d_mutex;
        BaseConnection::Ptr connect; ///< 发现者关联的客户端连接
        std::vector<std::string> methods; ///< 发现过的服务名称
        Discoverer(const BaseConnection::Ptr& conn)
            :connect(conn)
            {}
        void AppendMethod(const std::string& method){
            std::unique_lock<std::mutex> lock(d_mutex);
            methods.emplace_back(method);
        }
    };
    // 当每次客户端进行服务发现时新增发现者，新增服务名称
    Discoverer::Ptr AddDiscoverer(const BaseConnection::Ptr& conn, const std::string& method){
        Discoverer::Ptr discoverer;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _conns.find(conn);
            if(it != _conns.end()){
                discoverer = it->second;
            }
            else{
                discoverer = std::make_shared<Discoverer>(conn);
                _conns.emplace(conn, discoverer);
            }
            auto& discoverers = _discoverers[method];
            discoverers.insert(discoverer);
        }
        discoverer->AppendMethod(method);
        return discoverer;
    }
    // 发现者客户端断开连接时，找到发现者信息，删除关联数据
    void DelDiscoverer(const BaseConnection::Ptr& conn){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _conns.find(conn);
        if(it == _conns.end()){
            LOG_INFO("当前删除的客户端非服务发现者");
            return;
        }
        for(auto& method : it->second->methods){
            auto& discoverers = _discoverers[method];
            discoverers.erase(it->second);
        }
        _conns.erase(conn);
    }
    // 当有一个新的服务提供者上线，进行上线通知
    void OnlineNotity(const std::string& method, const Address& host){
        return __Notify(method, host, ServiceOperType::SERVICE_ONLINE);
    }
    // 当有一个服务提供者断开连接，进行下线通知
    void OfflineNotity(const std::string& method, const Address& host){
        return __Notify(method, host, ServiceOperType::SERVICE_OFFLINE);
    }
private:
    void __Notify(const std::string& method, const Address& host, ServiceOperType type){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _discoverers.find(method);
        if(it == _discoverers.end()){
            LOG_INFO("请求的服务'{}' 没有发现者", method);
            return ;
        }
        auto msg_req = MessageFactory::Create<ServiceRequest>();
        msg_req->SetId(Uuid::GetUuid());
        msg_req->SetMessType(MessType::REQUEST_SERVICE);
        msg_req->SetMethod(method);
        msg_req->SetHostMeassage(host);
        msg_req->SetServiceOperType(type);
        for(auto &discoverer : it->second){
            discoverer->connect->Send(msg_req);
        }
    }
private:
    std::mutex _mutex;
    std::unordered_map<std::string, std::set<Discoverer::Ptr>> _discoverers; ///< hash<method, vector<discover>>
    std::unordered_map<BaseConnection::Ptr, Discoverer::Ptr> _conns; ///< hash<conn, discover>
};
/**
 * @brief Provider-Discoverer 联合管理类
 * 
 */
class PDManager{
public:
    using Ptr = std::shared_ptr<PDManager>;
    PDManager()
        :_providers(std::make_shared<ProviderManager>())
        ,_discoverers(std::make_shared<DiscovererManager>())
        {}
    /**
     * @brief 服务请求接口：服务注册、服务发现
     * 
     * @param conn 
     * @param msg 
     * @details 
     *      服务注册：1. 新增服务提供者 2.进行服务上线通知
     *      服务发现：1. 新增服务发现者 
     */
    void OnServiceRequest(const BaseConnection::Ptr& conn, const ServiceRequest::Ptr& msg){
        
        auto otype = msg->ServiceOperType();
        if(otype == ServiceOperType::SERVICE_REGISRY){
            // 服务注册
            _providers->AddProvider(conn, msg->HostMeassage(), msg->Method());
            _discoverers->OnlineNotity(msg->Method(), msg->HostMeassage());
            __RegistryResponse(conn, msg);
        }
        else if(otype == ServiceOperType::SERVICE_DISCOVERY){
            // 服务发现
            _discoverers->AddDiscoverer(conn, msg->Method());
            __DiscoveryResponse(conn, msg);
        }
        else{
            LOG_ERROR("收到服务操作请求，但是操作类型错误");
            __ErrorResponse(conn, msg);
        }
    }
    void OnConnShutdown(const BaseConnection::Ptr& conn){
        auto provider = _providers->GetProvider(conn);
        if(provider.get() != nullptr){
            // 如果断开的连接就是提供者, 将提供的方法注销掉
            for(auto& method : provider->methods){
                _discoverers->OfflineNotity(method, provider->host);
            }
            _providers->DelProvider(conn);
        }
        // 服务发现者删除内部做了判断，如果不是发现者就直接返回
        _discoverers->DelDiscoverer(conn);
    }
private:
    void __RegistryResponse(const BaseConnection::Ptr& conn, const ServiceRequest::Ptr& msg){
        auto msg_rsp = MessageFactory::Create<ServiceResponse>();
        msg_rsp->SetId(msg->Rid());
        msg_rsp->SetMessType(MessType::RESPONSE_SERVICE);
        msg_rsp->SetRcode(ResCode::RCODE_OK);
        msg_rsp->SetServiceOperType(ServiceOperType::SERVICE_REGISRY);
        conn->Send(msg_rsp);
    }
    void __DiscoveryResponse(const BaseConnection::Ptr& conn, const ServiceRequest::Ptr& msg){
        auto msg_rsp = MessageFactory::Create<ServiceResponse>();
        auto hosts = _providers->GetMethodHosts(msg->Method());
        msg_rsp->SetId(msg->Rid());
        msg_rsp->SetMessType(MessType::RESPONSE_SERVICE);
        msg_rsp->SetServiceOperType(ServiceOperType::SERVICE_DISCOVERY);

        if(hosts.empty()){
            msg_rsp->SetRcode(ResCode::RCODE_NOT_FOUND_SERVICE);
            LOG_ERROR("无主机提供此{}服务错误", msg->Method());
            return conn->Send(msg_rsp);
        }
        
        msg_rsp->SetRcode(ResCode::RCODE_OK);
        msg_rsp->SetMethod(msg->Method());
        msg_rsp->SetHosts(hosts);
        conn->Send(msg_rsp);
    }
    void __ErrorResponse(const BaseConnection::Ptr& conn, const ServiceRequest::Ptr& msg){
        auto msg_rsp = MessageFactory::Create<ServiceResponse>();
        msg_rsp->SetId(msg->Rid());
        msg_rsp->SetMessType(MessType::RESPONSE_SERVICE);
        msg_rsp->SetRcode(ResCode::RCODE_INVALID_OPTYPE);
        msg_rsp->SetServiceOperType(ServiceOperType::SERVICE_UNKNOW);
        conn->Send(msg_rsp);
    }
private:
    ProviderManager::Ptr _providers;
    DiscovererManager::Ptr _discoverers;
};
}