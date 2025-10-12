#pragma once
#include "../common/Uuid.hpp"
#include "Requestor.hpp"
#include "unordered_set"

using namespace base;

namespace client{
class Provider{
public:
    using Ptr = std::shared_ptr<Provider>;
    bool RegistryMethod(const BaseConnection::Ptr& conn, const std::string& method, const Address& host){
        auto msg_req = MessageFactory::Create<ServiceRequest>();
        msg_req->SetId(Uuid::GetUuid());
        msg_req->SetMessType(MessType::REQUEST_SERVICE);
        msg_req->SetMethod(method);
        msg_req->SetHostMeassage(host);
        msg_req->SetServiceOperType(ServiceOperType::SERVICE_REGISRY);
        BaseMessage::Ptr msg_rsp;
        auto ret = _requestor->Send(conn, msg_req, msg_rsp);
        if(ret == false){
            LOG_ERROR("{} 服务注册失败", method);
            return false;
        }
        auto service_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
        if(service_rsp.get() == nullptr){
            LOG_ERROR("响应类型向下转型失败!");
            return false;
        }
        if(service_rsp->Rcode() != ResCode::RCODE_OK){
            LOG_ERROR("服务注册失败, 原因: {}", GetErrorReason(service_rsp->Rcode()));
            return false;
        }
        return true;
    }
private:
    Requestor::Ptr _requestor;
};

class MethodHost{
public:
    using Ptr = std::shared_ptr<MethodHost>;
    MethodHost():_index(0){}
    MethodHost(const std::vector<Address>& hosts)
    :_hosts(hosts), _index(0)
    {}
    void AppendHost(const Address& host){
        std::unique_lock<std::mutex> lock(_mutex);
        _hosts.emplace_back(host);
    }
    Address ChooseHost(){
        std::unique_lock<std::mutex> lock(_mutex);
        size_t pos = _index++ % _hosts.size(); // RR轮询
        return _hosts[pos];
    }
    void RemoveHost(const Address& host){
        std::unique_lock<std::mutex> lock(_mutex);
        for(auto it = _hosts.begin(); it!=_hosts.end(); ++it){
            if(*it == host){
                _hosts.erase(it);
                break;
            }
        }
    }
    bool Empty(){
        return _hosts.empty();
    }
private:
    std::mutex _mutex;
    size_t _index;
    std::vector<Address> _hosts; 
};

class Discoverer{
public:
    using Ptr = std::shared_ptr<Discoverer>;
    using OfflineCallback = std::function<void(const Address&)>;
    Discoverer(const Requestor::Ptr& requestor, const OfflineCallback& callback)
        :_requestor(requestor), _offline_callback(callback) {}
    bool ServiceDiscovery(const BaseConnection::Ptr& conn, const std::string& method, Address& host){
        {
            // 当前服务保管的提供者信息存在，则直接返回地址
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _method_hosts.find(method);
            if(it != _method_hosts.end()){
                if(!it->second->Empty()){
                    host = it->second->ChooseHost();
                    return true;
                }
            }   
        }
        // 当前服务提供者为空
        LOG_INFO("当前服务提供者为空，添加提供者");
        auto msg_req = MessageFactory::Create<ServiceRequest>();
        msg_req->SetId(Uuid::GetUuid());
        msg_req->SetMessType(MessType::REQUEST_SERVICE);
        msg_req->SetMethod(method);
        msg_req->SetHostMeassage(host);
        msg_req->SetServiceOperType(ServiceOperType::SERVICE_REGISRY);
        BaseMessage::Ptr msg_rsp;
        auto ret = _requestor->Send(conn, msg_req, msg_rsp);
        if(ret == false){
            LOG_ERROR("服务发现失败");
            return false;
        }
        auto service_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
        if(!service_rsp){
            LOG_ERROR("服务发现失败!响应类型转换失败");
            return false;
        }
        if(service_rsp->Rcode() == ResCode::RCODE_OK){
            LOG_ERROR("服务发现失败！原因: {}", GetErrorReason(service_rsp->Rcode()));
            return false;
        }
        // 能走到这里说明当前是没有对应的服务提供主机的
        std::unique_lock<std::mutex> lock(_mutex);
        auto method_hosts = std::make_shared<MethodHost>(service_rsp->Hosts());
        if(method_hosts->Empty()){
            LOG_ERROR("服务发现失败!没有提供该服务的主机");
            return false;
        }
        _method_hosts[method] = method_hosts;

        host = method_hosts->ChooseHost();
        return true;
    }
    // 提供给Dispatcher模块进行服务上下线请求处理的回调函数
    void OnserviceRequest(const BaseConnection::Ptr& conn, const ServiceRequest::Ptr& msg){
        auto optype = msg->ServiceOperType();
        auto method = msg->Method();
        std::unique_lock<std::mutex> lock(_mutex);
        if(optype==ServiceOperType::SERVICE_ONLINE){
            // 上线请求
            auto it = _method_hosts.find(method);
            if(it == _method_hosts.end()){
                // 没找到，新增地址信息
                auto method_host = std::make_shared<MethodHost>();
                method_host->AppendHost(msg->HostMeassage());
                _method_hosts[method] = method_host;
            }
            else{
                // 找到了，直接添加
                it->second->AppendHost(msg->HostMeassage());
            }
        }
        else if(optype==ServiceOperType::SERVICE_OFFLINE){
            // 下线请求
            auto it = _method_hosts.find(method);
            if(it == _method_hosts.end()){
                return ;
            }
            it->second->RemoveHost(msg->HostMeassage());
            _offline_callback(msg->HostMeassage());
        }
        else{
            LOG_ERROR("错误的服务类型");
        }
    }
private:
    std::mutex _mutex;
    std::unordered_map<std::string, MethodHost::Ptr> _method_hosts;
    Requestor::Ptr  _requestor;
    OfflineCallback _offline_callback;
};

}