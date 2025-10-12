#pragma once

#include "Requestor.hpp"
#include "../common/Uuid.hpp"

namespace client{
class RpcCaller{
public:
    using Ptr = std::shared_ptr<RpcCaller>;
    using JsonAsyncResponse = std::future<Json::Value>;
    using JsonResponseCallback = std::function<void(const Json::Value&)>;
public:
    RpcCaller(const Requestor::Ptr& requestor):_requestor(requestor){}
    /**
     * @brief: 三种请求调用- 同步、异步
     */
    // Requestor中的Send里边的回调是针对BaseMessage进行处理的
    //用于RpcCaller中针对结果的处理是针对RpcResponse里边的result进行的
    bool Call(const BaseConnection::Ptr& conn, const std::string& method,
            const Json::Value& params, Json::Value& result){
        // 1. 组织请求
        LOG_DEBUG("同步调用RpcCaller::Call");

        auto req_msg = MessageFactory::Create<RpcRequest>();
        req_msg->SetId(Uuid::GetUuid());
        req_msg->SetMessType(MessType::REQUEST_RPC);
        req_msg->SetMethod(method);
        req_msg->SetParams(params);
        // 2. 发送请求
        BaseMessage::Ptr rsp_msg;
        bool ret = _requestor->Send(conn, req_msg, rsp_msg);
        if(ret == false){
            LOG_ERROR("同步Rpc请求失败");
            return false;
        }
        // 3. 等待响应
        auto rpc_res = std::dynamic_pointer_cast<RpcResponse>(rsp_msg);
        if(!rpc_res){
            LOG_ERROR("rpc响应类型转换失败");
            return false;
        }
        if(rpc_res->Rcode() != ResCode::RCODE_OK){
            LOG_ERROR("rpc请求出错: {}", GetErrorReason(rpc_res->Rcode()));
            return false;
        }
        result = rpc_res->Result();
        return true;
    }
    bool Call(const BaseConnection::Ptr& conn, const std::string& method,
        const Json::Value& params, JsonAsyncResponse& result){
        LOG_DEBUG("异步调用RpcCaller::Call");
        // 向服务器发送异步回调请求，设置回调函数，回调函数中会传入一个promise对象，在回调函数中去对promise设置数据
        // 1. 组织请求
        auto req_msg = MessageFactory::Create<RpcRequest>();
        req_msg->SetId(Uuid::GetUuid());
        req_msg->SetMessType(MessType::REQUEST_RPC);
        req_msg->SetMethod(method);
        req_msg->SetParams(params);
        
        // std::promise<Json::Value> 是局部的
        auto json_promise = std::make_shared<std::promise<Json::Value>>();
        result = json_promise->get_future();
        Requestor::RequestCallback cb = std::bind(&RpcCaller::Callback, this, json_promise, std::placeholders::_1);
        // 2. 发送请求
        bool ret = _requestor->Send(conn, req_msg, cb);
        if(ret == false){
            LOG_ERROR("同步Rpc请求失败");
            return false;
        }
        return true;
    }
    
    bool Call(const BaseConnection::Ptr& conn, const std::string& method,
            const Json::Value& params, const JsonResponseCallback& callback){
        LOG_DEBUG("回调执行RpcCaller::Call");
        auto req_msg = MessageFactory::Create<RpcRequest>();
        req_msg->SetId(Uuid::GetUuid());
        req_msg->SetMessType(MessType::REQUEST_RPC);
        req_msg->SetMethod(method);
        req_msg->SetParams(params);

        Requestor::RequestCallback cb = std::bind(&RpcCaller::CallbackRun, this, callback, std::placeholders::_1);
        bool ret = _requestor->Send(conn, req_msg, cb);
        if(ret == false){
            LOG_ERROR("回调Rpc请求失败");
            return false;
        }
        return true;
    }
private:
    void CallbackRun(const JsonResponseCallback& callback, const BaseMessage::Ptr& msg){
         auto rpc_res = std::dynamic_pointer_cast<RpcResponse>(msg);
        if(!rpc_res){
            LOG_ERROR("rpc响应类型转换失败");
            return;
        }
        if(rpc_res->Rcode() != ResCode::RCODE_OK){
            LOG_ERROR("rpc异步请求出错: {}", GetErrorReason(rpc_res->Rcode()));
            return ;
        }
        callback(rpc_res->Result());
    }

    void Callback(std::shared_ptr<std::promise<Json::Value>> result, const BaseMessage::Ptr& msg){
        auto rpc_res = std::dynamic_pointer_cast<RpcResponse>(msg);
        if(!rpc_res){
            LOG_ERROR("rpc响应类型转换失败");
            return;
        }
        if(rpc_res->Rcode() != ResCode::RCODE_OK){
            LOG_ERROR("rpc异步请求出错: {}", GetErrorReason(rpc_res->Rcode()));
            return ;
        }
        result->set_value(rpc_res->Result());
    }
private:
    Requestor::Ptr _requestor;
};

}