#pragma once
#include <string>
#include <functional>
#include <memory>
#include "Fileds.hpp"

namespace base
{
    using namespace common;
class BaseMessage{
public:
    using Ptr = std::shared_ptr<BaseMessage>;
    virtual ~BaseMessage() = default;
    virtual void SetId(const std::string& id){
        _rid = id;
    }
    virtual const std::string Rid(){
        return _rid;
    }
    virtual void SetMessType(MessType mtype){
        _mtype = mtype;
    }
    virtual MessType GetMessType() {return _mtype;}

    /// @brief 纯虚接口
    virtual std::string Serialize() = 0;
    virtual bool Unserialize(const std::string& msg) = 0;
    virtual bool Check() = 0;
protected:
    MessType _mtype;
    std::string _rid;
};

class BaseBuffer{
public:
    using Ptr = std::shared_ptr<BaseBuffer>;
    virtual ~BaseBuffer() = default;
    virtual size_t ReadableSize() = 0;
    virtual int32_t PeekInt32() = 0;
    virtual void RetrieveInt32() = 0;
    virtual int32_t ReadInt32() = 0;
    virtual std::string RetriveAsString(size_t len) = 0;
};

class BaseProtocol{
public:
    using Ptr = std::shared_ptr<BaseProtocol>;
    virtual ~BaseProtocol() = default;
    virtual bool CanProcessed(const BaseBuffer::Ptr& buffer) = 0;
    virtual bool OnMessage(const BaseBuffer::Ptr& buffer, BaseMessage::Ptr& msg) = 0;
    virtual std::string Serialize(const BaseMessage::Ptr& msg) = 0;
};
class BaseConnection{
public:
    using Ptr = std::shared_ptr<BaseConnection>;
    virtual ~BaseConnection() = default;
    virtual void Send(const BaseMessage::Ptr& buffer) = 0;
    virtual void Shutdown() = 0;
    virtual bool IsConnected() = 0;
};
using ConnectionCallBack = std::function<void(const BaseConnection::Ptr&)>;
using CloseCallBack = std::function<void(const BaseConnection::Ptr&)>;
using MessageCallBack = std::function<void(const BaseConnection::Ptr&, BaseMessage::Ptr&)>;

class BaseServer{
public:
    using Ptr = std::shared_ptr<BaseServer>;
    virtual ~BaseServer() = default;
    virtual void SetConnectionCallBack(const ConnectionCallBack& cb){_cb_connection = cb;}
    virtual void SetCloseCallBack(const CloseCallBack& cb) {_cb_close = cb;}
    virtual void SetMessageCallBack(const MessageCallBack& cb) {_cb_message = cb;}
    virtual void Start() = 0;
protected:
    ConnectionCallBack _cb_connection;
    CloseCallBack _cb_close;
    MessageCallBack _cb_message;
};

class BaseClient{
public:
    using Ptr = std::shared_ptr<BaseClient>;
    virtual ~BaseClient() = default;
    virtual void SetConnectionCallBack(const ConnectionCallBack& cb){_cb_connection = cb;}
    virtual void SetCloseCallBack(const CloseCallBack& cb) {_cb_close = cb;}
    virtual void SetMessageCallBack(const MessageCallBack& cb) {_cb_message = cb;}
    virtual void Connect() = 0;
    virtual void Shutdown() = 0;
    virtual bool Send(const BaseMessage::Ptr& msg) = 0;
    virtual bool IsConnected() = 0;
    virtual BaseConnection::Ptr GetConnection() = 0;
protected:
    ConnectionCallBack _cb_connection;
    CloseCallBack _cb_close;
    MessageCallBack _cb_message;
};

} // namespace base

