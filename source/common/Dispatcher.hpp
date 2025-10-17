#pragma once
#include "Net.hpp"
#include "Message.hpp"
#include <functional>

namespace base{
class Callback{
public:
    using Ptr = std::shared_ptr<Callback>;
    virtual void OnMessage(const BaseConnection::Ptr& conn, BaseMessage::Ptr& msg) = 0;
};

template<typename T>
class CallbackT : public Callback{
public:
    using Ptr = std::shared_ptr<CallbackT<T>>;
    using MessageCallback = std::function<void(const BaseConnection::Ptr& conn, std::shared_ptr<T>& msg)>;
    CallbackT(const MessageCallback& handler):_handler(handler){}
    void OnMessage(const BaseConnection::Ptr& conn, BaseMessage::Ptr& msg) override{
        auto type_msg = std::dynamic_pointer_cast<T>(msg);
        _handler(conn, type_msg);
    }
private:
    MessageCallback _handler;
};
class Dispatcher{
public:
    using Ptr = std::shared_ptr<Dispatcher>;
    template<class T>
    void RegisterHandler(const MessType& type, const typename CallbackT<T>::MessageCallback& handler){
        std::unique_lock<std::mutex> lock(_mutex);
        auto cb = std::make_shared<CallbackT<T>>(handler);
        _handlers.emplace(type, cb);
    }
    void OnMessage(const BaseConnection::Ptr& conn, BaseMessage::Ptr& msg){
        // 收到消息类型对应的业务处理函数
        std::unique_lock<std::mutex> lock(_mutex);
        
        auto it = _handlers.find(msg->GetMessType());
        if(it!=_handlers.end()){
            return it->second->OnMessage(conn, msg);
        }
        LOG_ERROR("收到未知类型的消息!");
        conn->Shutdown();
    }
private:
    std::mutex _mutex;
    std::unordered_map<MessType, Callback::Ptr> _handlers;
};

}