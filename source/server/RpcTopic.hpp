#pragma once
#include "../common/Net.hpp"
#include "../common/Message.hpp"
#include <unordered_set>


using namespace base;
namespace server{
class TopicManager{
public:
    using Ptr = std::shared_ptr<TopicManager>;

    void OnTopicRequest(const BaseConnection::Ptr& conn, const TopicRequest::Ptr& msg){
        auto topic_otype = msg->TopicOperType();
        bool ret = true;
        switch(topic_otype){
            // 主题创建
            case TopicOperType::TOPIC_CREATE: __TopicCreate(conn, msg); break;
            // 主题删除
            case TopicOperType::TOPIC_REMOVE: __TopicRemove(conn, msg); break;
            // 主题订阅
            case TopicOperType::TOPIC_SUBSCRIBE: 
                ret = __TopicSubscribe(conn, msg); 
                break;
            // 主题取消订阅
            case TopicOperType::TOPIC_CANCEL: __TopicCancel(conn, msg); break;
            // 主题发布
            case TopicOperType::TOPIC_PUBLISH: ret = __TopicPublish(conn, msg); break;
            default:
                LOG_ERROR("错误的主题请求类型");
                return __ErrorResponse(conn, msg, ResCode::RCODE_INVALID_OPTYPE);
        }
        if(!ret){
            return __ErrorResponse(conn, msg, ResCode::RCODE_NOT_FOUND_TOPIC);
        }
        return __TopicResponse(conn, msg);
    }
    // 订阅者在断开连接时，删除其关联的数据
    void OnShutDown(const BaseConnection::Ptr& conn){
        // 消息发布者断开连接，不需要任何操作；消息订阅者断开需要删除管理数据
        // 1.判断断开连接的是否是订阅者，不是的话直接返回退出
        std::vector<Topic::Ptr> topics;
        Subscriber::Ptr subscriber;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _subscribers.find(conn);
            if(it == _subscribers.end()){
                return ; // 断开的连接不是一个消息订阅者
            }
            subscriber = it->second;
            // 2.获取到订阅者退出，受影响的主题对象
            for(auto& topic_name : it->second->topics){
                auto topic_iter = _topics.find(topic_name);
                if(topic_iter == _topics.end()) continue;
                topics.emplace_back(topic_iter->second);
            }
            // 4.从订阅者映射信息中，删除订阅者
            _subscribers.erase(it);
        }
        // 3.从主题对象中，移除订阅者
        for(auto& topic : topics){
            topic->RemoveSubscriber(subscriber);
        }
    }
private:
    void __ErrorResponse(const BaseConnection::Ptr& conn, const TopicRequest::Ptr& msg, ResCode rcode){
        auto msg_rsp = MessageFactory::Create<TopicResponse>();
        msg_rsp->SetId(msg->Rid());
        msg_rsp->SetMessType(MessType::RESPONSE_TOPIC);
        msg_rsp->SetRcode(rcode);
        conn->Send(msg_rsp);
    }
    void __TopicResponse(const BaseConnection::Ptr& conn, const TopicRequest::Ptr& msg){
        auto msg_rsp = MessageFactory::Create<TopicResponse>();
        msg_rsp->SetId(msg->Rid());
        msg_rsp->SetMessType(MessType::RESPONSE_TOPIC);
        msg_rsp->SetRcode(ResCode::RCODE_OK);
        conn->Send(msg_rsp);
    }
    void __TopicCreate(const BaseConnection::Ptr& conn, const TopicRequest::Ptr& msg){
        std::unique_lock<std::mutex> lock(_mutex);
        auto topic_name = msg->TopicKey();
        auto topic = std::make_shared<Topic>(topic_name);
        _topics.emplace(topic_name, topic);
    }
    void __TopicRemove(const BaseConnection::Ptr& conn, const TopicRequest::Ptr& msg){
        auto topic_name = msg->TopicKey();
        std::unordered_set<Subscriber::Ptr> subscribers;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            // 删除主题之前找到收到影响的订阅者
            auto it = _topics.find(topic_name);
            if(it == _topics.end()){ return ;}
            subscribers = it->second->subscribers;
            
            _topics.erase(it); //删除当前的主题映射关系
        }
        for(auto& subscriber : subscribers){
            subscriber->RemoveTopic(topic_name);
        }
    }
    bool __TopicSubscribe(const BaseConnection::Ptr& conn, const TopicRequest::Ptr& msg){
        // 1.先找出主题对象，以及订阅者对象
        // 如果没有找到主题--就要报错；但是如果没有找到订阅者对象，就要构造一个订阅者对象
        Topic::Ptr topic;
        Subscriber::Ptr subscriber;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto topic_iter = _topics.find(msg->TopicKey());

            if(topic_iter == _topics.end()){ return false;}

            topic = topic_iter->second;
            auto sub_iter = _subscribers.find(conn);
            if(sub_iter != _subscribers.end()){
                subscriber = sub_iter->second;
            }
            else{
                subscriber = std::make_shared<Subscriber>(conn);
                _subscribers.emplace(conn, subscriber);
            }
        }
        // 2.在主题对象中，新增一个订阅者对象关联的连接，在订阅者对象中新增一个订阅的主题
        topic->AppendSubscriber(subscriber);
        subscriber->AppendTopic(msg->TopicKey());
        return true;
    }
    void __TopicCancel(const BaseConnection::Ptr& conn, const TopicRequest::Ptr& msg){
        // 1.先找出主题对象，以及订阅者对象
        Topic::Ptr topic;
        Subscriber::Ptr subscriber;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto topic_iter = _topics.find(msg->TopicKey());

            if(topic_iter != _topics.end()){
                topic = topic_iter->second;
            }

            auto sub_iter = _subscribers.find(conn);
            if(sub_iter != _subscribers.end()){
                subscriber = sub_iter->second;
            }
            else{
                return ;
            }
        }
        // 2.在主题对象中删除当前订阅者连接，从订阅者信息中删除所订阅的主题名称
        if(subscriber) subscriber->RemoveTopic(msg->TopicKey());
        if(topic && subscriber) topic->RemoveSubscriber(subscriber);
    }
    bool __TopicPublish(const BaseConnection::Ptr& conn, const TopicRequest::Ptr& msg){
        Topic::Ptr topic;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto topic_iter = _topics.find(msg->TopicKey());

            if(topic_iter == _topics.end()){ return false;}

            topic = topic_iter->second;
        }
        topic->PushMessage(msg);
        return true;
    }
private: 
    struct Subscriber{
        using Ptr = std::shared_ptr<Subscriber>;
        std::mutex s_mutex;
        BaseConnection::Ptr conn;
        std::unordered_set<std::string> topics; ///< 订阅者订阅的主题

        Subscriber(const BaseConnection::Ptr& c):conn(c){}
        // 订阅主题
        void AppendTopic(const std::string& topic_name){
            std::unique_lock<std::mutex> lock(s_mutex);
            topics.insert(topic_name);
        }
        // 删除主题 或者 取消订阅
        void RemoveTopic(const std::string& topic_name){
            std::unique_lock<std::mutex> lock(s_mutex);
            topics.erase(topic_name);
        }
    };
    struct Topic{
        using Ptr = std::shared_ptr<Topic>;
        std::mutex t_mutex;
        std::string topic_name;
        std::unordered_set<Subscriber::Ptr> subscribers; ///< 当前主题的订阅者

        Topic(const std::string& topic_name):topic_name(topic_name) {}
        // 新增订阅
        void AppendSubscriber(const Subscriber::Ptr& sub){
            std::unique_lock<std::mutex> lock(t_mutex);
            subscribers.insert(sub);
        }
        // 删除订阅
        void RemoveSubscriber(const Subscriber::Ptr& sub){
            std::unique_lock<std::mutex> lock(t_mutex);
            subscribers.erase(sub);
        }
        // 收到消息发布请求
        void PushMessage(const BaseMessage::Ptr& msg){
            std::unique_lock<std::mutex> lock(t_mutex);
            for(auto& subscriber : subscribers){
                subscriber->conn->Send(msg); // 广播消息
            }
        }
    };
private:
    std::mutex _mutex;
    std::unordered_map<std::string, Topic::Ptr> _topics;
    std::unordered_map<BaseConnection::Ptr, Subscriber::Ptr> _subscribers;
};
}