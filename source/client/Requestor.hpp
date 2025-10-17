#pragma once

#include "../common/Net.hpp"
#include "../common/Message.hpp"
#include <future>

using namespace base;

namespace client{
class Requestor{
public:
    using Ptr = std::shared_ptr<Requestor>;
    using AsyncResponse = std::future<BaseMessage::Ptr>;
    using RequestCallback = std::function<void(const BaseMessage::Ptr&)>;
    struct RequestDescribe
    {
        using Ptr = std::shared_ptr<RequestDescribe>;
        BaseMessage::Ptr request;
        RequestType rtype;
        std::promise<BaseMessage::Ptr> response;
        RequestCallback callback;


        void SetRequest(const BaseMessage::Ptr& req){this->request=req;}
        void SetRequestType(RequestType rtype){this->rtype=rtype;}
        void SetCallback(const RequestCallback& callback){this->callback=callback;}
        AsyncResponse GetAsyncResponse(){return response.get_future();}
    };
public:
    void OnResponse(const BaseConnection::Ptr& conn, BaseMessage::Ptr& msg){
        std::string rid = msg->Rid();
        RequestDescribe::Ptr rdp = __GetDescribe(rid);
        if(rdp.get() == nullptr){
            LOG_ERROR("收到响应 '{}', 但是未找到对应的请求描述", rid);
            return;
        }
        if(rdp->rtype == RequestType::REQUEST_ASYNC){
            rdp->response.set_value(msg);
        }
        else if(rdp->rtype == RequestType::REQUEST_CALLBACK){
            if(rdp->callback) rdp->callback(msg);
        }
        else{
            LOG_ERROR("请求类型未知");
        }
        __DelDescribe(rid); // 避免内存泄漏
    }
    /**
     * @brief: 两种响应方式
     */
    bool Send(const BaseConnection::Ptr& conn, const BaseMessage::Ptr& req, AsyncResponse &async_rsq){
        RequestDescribe::Ptr rdp = __NewDescribe(req, RequestType::REQUEST_ASYNC);
        if(rdp.get() == nullptr){
            LOG_ERROR("构造请求描述对象失败");
            return false;
        }
        conn->Send(req);
        async_rsq = rdp->GetAsyncResponse();
        return true;
    }
    bool Send(const BaseConnection::Ptr& conn, const BaseMessage::Ptr& req, BaseMessage::Ptr& rsp){
        AsyncResponse rsp_future;
        bool ret = Send(conn, req, rsp_future);
        if(ret == false) return false;
        rsp = rsp_future.get();
        return true;
    }

    bool Send(const BaseConnection::Ptr& conn, const BaseMessage::Ptr& req, RequestCallback &callback){
        RequestDescribe::Ptr rdp = __NewDescribe(req, RequestType::REQUEST_CALLBACK, callback);
        if(rdp.get() == nullptr){
            LOG_ERROR("构造请求描述对象失败");
            return false;
        }
        // callback = rdp->callback;
        conn->Send(req);
        return true;
    }
private:
    /**
     * @brief: request请求信息管理模块- 增删改查
     */
    RequestDescribe::Ptr __NewDescribe(const BaseMessage::Ptr& req, RequestType rtype,
                                    const RequestCallback &cb = RequestCallback()){
        std::unique_lock<std::mutex> lock(_mutex);
        RequestDescribe::Ptr rd = std::make_shared<RequestDescribe>();
        rd->SetRequest(req);
        rd->SetRequestType(rtype);
        if(rtype == RequestType::REQUEST_CALLBACK && cb){
            rd->SetCallback(cb);
        }
        LOG_INFO("新增请求描述信息: {}", req->Rid());
        _request_desc.emplace(req->Rid(), rd);
        return rd;
    }
    RequestDescribe::Ptr __GetDescribe(const std::string& rid){
        LOG_INFO("查找请求描述信息: {}", rid);
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _request_desc.find(rid);
        if(it == _request_desc.end()){
            return RequestDescribe::Ptr();
        }
        return it->second;
    }
    void __DelDescribe(const std::string& rid){
        std::unique_lock<std::mutex> lock(_mutex);
        _request_desc.erase(rid);
    }
private:
    std::mutex _mutex;
    std::unordered_map<std::string, RequestDescribe::Ptr> _request_desc; ///< id->request 映射表
};
}