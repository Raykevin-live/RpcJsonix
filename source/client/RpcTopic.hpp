#pragma once
#include "../common/Uuid.hpp"
#include "Requestor.hpp"
#include <unordered_set>

using namespace base;
namespace client{
class TopicManager{
public:
    using Ptr = std::shared_ptr<TopicManager>;
    using SubCallback = std::function<void(const std::string&, const std::string&)>;

    TopicManager(const Requestor::Ptr& requestor):_requestor(requestor){}
    bool Create(const BaseConnection::Ptr& conn, const std::string& key){
        return __CommonRequest(conn, key, TopicOperType::TOPIC_CREATE);
    }
    bool Remove(const BaseConnection::Ptr& conn, const std::string& key){
        return __CommonRequest(conn, key, TopicOperType::TOPIC_REMOVE);
    }
    bool Subscribe(const BaseConnection::Ptr& conn, const std::string& key, const SubCallback& cb){
        __AddSubscribe(key, cb);
        bool ret = __CommonRequest(conn, key, TopicOperType::TOPIC_SUBSCRIBE);
        if(ret == false){
            __DelSubscribe(key);
            return false;
        }
        return true;
    }
    bool Cancel(const BaseConnection::Ptr& conn, const std::string& key){
        __DelSubscribe(key);
        return __CommonRequest(conn, key, TopicOperType::TOPIC_CANCEL);
    }
    bool Publish(const BaseConnection::Ptr& conn, const std::string& key, const std::string& msg){
        return __CommonRequest(conn, key, TopicOperType::TOPIC_PUBLISH, msg);
    }

    void OnPublish(const BaseConnection::Ptr& conn, const TopicRequest::Ptr& msg){
        // 1. 从消息中取出操作类型进行判断，是否是消息请求
        auto type = msg->TopicOperType();
        if(type != TopicOperType::TOPIC_PUBLISH){
            LOG_ERROR("收到了错误类型的主题操作!");
            return ;
        }
        // 2. 取出消息主题名称，以及消息内容
        std::string topic_key = msg->TopicKey();
        std::string topic_msg = msg->TopicMeassage();
        // 3. 通过主题名称，查找对应主题的回调函数，有则进行处理，无则报错
        auto callback = __GetSubscribe(topic_key);
        if(!callback){
            LOG_ERROR("收到了 {} 主题消息，但是该消息无主题处理回调!", topic_key);
            return ;
        }
        callback(topic_key, topic_msg);
    }
private:
    bool __CommonRequest(const BaseConnection::Ptr& conn, const std::string& key, 
                TopicOperType type, const std::string& msg = ""){
        // 1.构造请求对象，并填充数据
        auto msg_req = MessageFactory::Create<TopicRequest>();
        msg_req->SetId(common::Uuid::GetUuid());
        msg_req->SetMessType(MessType::REQUEST_TOPIC);
        msg_req->SetTopicOperType(type);
        msg_req->SetTopicKey(key);
        if (type == TopicOperType::TOPIC_PUBLISH) {
            msg_req->SetTopicMeassage(msg);
        }
        // 2.向服务端发送请求，等待响应
        BaseMessage::Ptr msg_rsp;
        bool ret = _requestor->Send(conn, msg_req, msg_rsp);
        if(ret == false){
            LOG_ERROR("{}主题操作请求失败", key);
            return false;
        }
        // 3.判断请求是否成功
        auto topic_rsp_msg = std::dynamic_pointer_cast<TopicResponse>(msg_rsp);
        if(!topic_rsp_msg){
            LOG_ERROR("主题操作响应，向下类型转换失败!");
            return false;
        }
        if(topic_rsp_msg->Rcode() != ResCode::RCODE_OK){
            LOG_ERROR("主题操作请求出错: {}", GetErrorReason(topic_rsp_msg->Rcode()));
            return false;
        }
        return true;
    }
    void __AddSubscribe(const std::string& key, const SubCallback& cb){
        std::unique_lock<std::mutex> lock(_mutex);
        _topic_callbacks.emplace(key, cb);
    }
    void __DelSubscribe(const std::string& key){
        std::unique_lock<std::mutex> lock(_mutex);
        _topic_callbacks.erase(key);
    }
    const SubCallback __GetSubscribe(const std::string& key){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _topic_callbacks.find(key);
        if(it == _topic_callbacks.end()){
            return SubCallback();
        }
        return it->second;
    }
private:
    std::mutex _mutex;
    std::unordered_map<std::string, SubCallback> _topic_callbacks;
    Requestor::Ptr _requestor;
};
}