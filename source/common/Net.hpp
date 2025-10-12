#pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoopThread.h>
#include <mutex>
#include <unordered_map>
#include "Fileds.hpp"
#include "Abstract.hpp"
#include "Message.hpp"

namespace base{
class MuduoBuffer : public BaseBuffer{
public:
    using Ptr = std::shared_ptr<MuduoBuffer>;
    MuduoBuffer(muduo::net::Buffer* buf):_buf(buf){}
    virtual ~MuduoBuffer() = default;
    virtual size_t ReadableSize() override{
        return _buf->readableBytes();
    }
    virtual int32_t PeekInt32() override{ // 取4个字节但是不删除
        // 网络字节序转主机字节序：muduo库是一个网络库，从缓冲区取出一个4字节整形，会进行网络字节序的转换 
        return _buf->peekInt32();
    }
    virtual void RetrieveInt32() override{ // 删除缓冲区中的4个字节
        // 网络字节序转主机字节序：muduo库是一个网络库，从缓冲区取出一个4字节整形，会进行网络字节序的转换 
        return _buf->retrieveInt32();
    }
    virtual int32_t ReadInt32() override{
        return _buf->readInt32();
    }
    virtual std::string RetriveAsString(size_t len) override{
        return _buf->retrieveAsString(len);
    }
private:
    muduo::net::Buffer* _buf;
};

class BufferFactory{
public:
    template<typename ...Args>
    static BaseBuffer::Ptr Create(Args&& ...args){
        return std::make_shared<MuduoBuffer>(std::forward<Args>(args)...); /// 这里的... 是展开参数包的意思
    }
};

class LVProtocol : public BaseProtocol{
public:
//  |--len--|--value--|
//  |--len--|--mtype--|--id_len--|--id--|--body--|
    using Ptr = std::shared_ptr<LVProtocol>;
    virtual ~LVProtocol() = default;
    // 判断缓冲区中的数量是否足够一条消息的处理
    virtual bool CanProcessed(const BaseBuffer::Ptr& buffer) override{
        if(buffer->ReadableSize() < lenFieldsLength){
            return false;
        }
        int32_t total_len = buffer->PeekInt32();
        if(buffer->ReadableSize() < (total_len + lenFieldsLength)){
            return false;
        }
        return true;
    }
    virtual bool OnMessage(const BaseBuffer::Ptr& buffer, BaseMessage::Ptr& msg) override{
        // 调用此函数时，默认认为缓冲区中的数据足够一条完整的消息
        int32_t total_len = buffer->ReadInt32(); //读取总长度
        MessType mytype = (MessType)buffer->ReadInt32();  // 读取数据类型
        int32_t id_len = buffer->ReadInt32();  // 读取id长度
        int32_t body_len = total_len - id_len - idLenFieldsLength - mtypeFieldsLength;

        std::string id = buffer->RetriveAsString(id_len);
        std::string body = buffer->RetriveAsString(body_len);

        // 构造对象
        msg = MessageFactory::Create(mytype);
        if(msg.get() == nullptr){
            LOG_ERROR("消息类型错误， 构造消息对象失败!");
            return false;
        }
        bool ret = msg->Unserialize(body);
        if(ret == false){
            LOG_ERROR("消息正文反序列化失败!");
            return false;
        }

        msg->SetId(id);
        msg->SetMessType(mytype);
        LOG_DEBUG("消息构造成功");
        return true;
    }
    virtual std::string Serialize(const BaseMessage::Ptr& msg) override{
    //  |--len--|--mtype--|--id_len--|--id--|--body--|
        std::string body = msg->Serialize();
        std::string id = msg->Rid();
        // 字节序转换
        auto mtype = htonl((int32_t)msg->GetMessType()); 
        auto id_len = htonl(id.size());
        auto h_total_len = mtypeFieldsLength + idLenFieldsLength + id.size() + body.size();
        auto nl_total_len = htonl(h_total_len);
        std::string result;
        result.reserve(h_total_len);
        result.append((char*)&nl_total_len, lenFieldsLength);
        result.append((char*)&mtype, mtypeFieldsLength);
        result.append((char*)&id_len, idLenFieldsLength);
        result.append(id);
        result.append(body);
        return result;
    }
private:
    static const int32_t lenFieldsLength = 4;
    static const int32_t mtypeFieldsLength = 4;
    static const int32_t idLenFieldsLength = 4;
};
class ProtocolFactory{
public:
    template<typename ...Args>
    static BaseProtocol::Ptr Create(Args&& ...args){
        return std::make_shared<LVProtocol>(std::forward<Args>(args)...); /// 这里的..., 是展开参数包的意思
    }
};

class MuduoConnection : public BaseConnection{
public:
using Ptr = std::shared_ptr<MuduoConnection>;
    virtual ~MuduoConnection() = default;
    MuduoConnection(const BaseProtocol::Ptr& protocol, const muduo::net::TcpConnectionPtr& conn)
    :_protocol(protocol), _conn(conn) {}
    virtual void Send(const BaseMessage::Ptr& msg) override{
        std::string body = _protocol->Serialize(msg);
        LOG_DEBUG("序列化发送的消息: \n{}", body);
        _conn->send(body);
    }
    virtual void Shutdown() override{
        _conn->shutdown();
    }
    virtual bool IsConnected() override{
        return _conn->connected();
    }
private:
    BaseProtocol::Ptr _protocol;
    muduo::net::TcpConnectionPtr _conn;
};
class ConnectionFactory{
public:
    template<typename ...Args>
    static BaseConnection::Ptr Create(Args&& ...args){
        return std::make_shared<MuduoConnection>(std::forward<Args>(args)...); /// 这里的... 是展开参数包的意思
    }
};

class MuduoServer : public BaseServer{
public:
    using Ptr = std::shared_ptr<MuduoServer>;
    MuduoServer(int16_t port)
    :_server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port),
        "MuduoServer", muduo::net::TcpServer::kNoReusePort),
    _protocol(ProtocolFactory::Create())
    {}
    virtual void Start() override{
        _server.setConnectionCallback(std::bind(&MuduoServer::OnConnection, this, std::placeholders::_1)); //参数绑定
        _server.setMessageCallback(std::bind(&MuduoServer::OnMessage, this, 
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3));
        _server.start();
        _baseloop.loop();
    }
private:
    void OnConnection(const muduo::net::TcpConnectionPtr& connect)
    {
       if(connect->connected())
        {
            LOG_INFO("连接建立!");
            auto muduo_conn = ConnectionFactory::Create(_protocol, connect);
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _conns.emplace(connect, muduo_conn);
            }
            if(_cb_connection) _cb_connection(muduo_conn);
        }
        else
        {
            LOG_INFO("连接断开!");
            BaseConnection::Ptr muduo_conn;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _conns.find(connect);
                if(it == _conns.end()){
                    return ;
                }
                muduo_conn = it->second;
                _conns.erase(connect);
            }
            if(_cb_close) _cb_close(muduo_conn);
        }
    }
    void OnMessage(const muduo::net::TcpConnectionPtr& connect, muduo::net::Buffer* buffer, muduo::Timestamp time)
    {
        auto base_buffer = BufferFactory::Create(buffer);
       
        while(1){
            if(_protocol->CanProcessed(base_buffer) == false){
                // 数据不足
                if(base_buffer->ReadableSize() > maxDataSize){
                    connect->shutdown();
                    LOG_ERROR("缓冲区数据过大");
                    return ;
                }
                LOG_DEBUG("数据不足");
                break;
            }
            LOG_DEBUG("缓冲区中数据可处理");
            BaseMessage::Ptr msg;
            bool ret = _protocol->OnMessage(base_buffer, msg);
            if(ret == false){
                connect->shutdown();
                LOG_ERROR("缓冲区数据错误");
                return ;
            }
            // 获取连接
             BaseConnection::Ptr base_conn;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _conns.find(connect);
                if(it == _conns.end()){
                    connect->shutdown();
                    return ;
                }
                base_conn = it->second;
            }
            LOG_DEBUG("消息回调函数执行");
            if(_cb_message) _cb_message(base_conn, msg);
        }
    }
private:
    static const int maxDataSize = (1<<16);
    BaseProtocol::Ptr _protocol;
    muduo::net::EventLoop _baseloop;
    muduo::net::TcpServer _server;
    std::mutex _mutex;
    std::unordered_map<muduo::net::TcpConnectionPtr, BaseConnection::Ptr> _conns;
};
class ServerFactory{
public:
    template<typename ...Args>
    static BaseServer::Ptr Create(Args&& ...args){
        return std::make_shared<MuduoServer>(std::forward<Args>(args)...); 
    }
};

class MuduoClient : public BaseClient{
public:
    using Ptr = std::shared_ptr<MuduoClient>;
    MuduoClient(const std::string& sip, uint16_t port)
    :_protocol(ProtocolFactory::Create()),
    _baseloop(_loopthread.startLoop()),
    _downlatch(1), // 初始化计数器为1，为0时被唤醒
    _client(_baseloop, muduo::net::InetAddress(sip, port), "MuduoClient")
    {}
    virtual void Connect() override{
         _client.setConnectionCallback(std::bind(&MuduoClient::OnConnection, this, std::placeholders::_1)); //参数绑定
        _client.setMessageCallback(std::bind(&MuduoClient::OnMessage, this, 
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3));
        // 连接服务器
        _client.connect();
        _downlatch.wait(); //等待
    }
    virtual void Shutdown() override{
        return _client.disconnect();
    }
    virtual bool Send(const BaseMessage::Ptr& msg)override{
        if(IsConnected() == false){
            LOG_ERROR("连接已断开");
            return false;
        }
        LOG_DEBUG("客户端发送消息");
        _conn->Send(msg);
        return true;
    }
    virtual bool IsConnected() override{
        return (_conn && _conn->IsConnected());
    }
    virtual BaseConnection::Ptr GetConnection() override{
        return _conn;
    }
private:
    void OnConnection(const muduo::net::TcpConnectionPtr& connect)
    {
       if(connect->connected())
        {
            LOG_INFO("连接建立!");
            _downlatch.countDown(); //计数--，为0时唤醒阻塞
            _conn = ConnectionFactory::Create(_protocol, connect);
        }
        else
        {
            LOG_INFO("连接断开!");
            _conn.reset();
        }
    }
    void OnMessage(const muduo::net::TcpConnectionPtr& connect, muduo::net::Buffer* buffer, muduo::Timestamp time)
    {
        LOG_DEBUG("服务器有数据到来, 开始处理!");
        auto base_buffer = BufferFactory::Create(buffer);
        while(1){
            if(_protocol->CanProcessed(base_buffer) == false){
                // 数据不足
                if(base_buffer->ReadableSize() > maxDataSize){
                    connect->shutdown();
                    LOG_ERROR("缓冲区数据过大");
                    return ;
                }
                LOG_DEBUG("数据量不足");
                break;
            }
            LOG_DEBUG("缓冲区消息可处理");
            BaseMessage::Ptr msg;
            bool ret = _protocol->OnMessage(base_buffer, msg);
            if(ret == false){
                connect->shutdown();
                LOG_ERROR("缓冲区数据错误");
                return ;
            }
            LOG_DEBUG("缓冲区中数据解析完毕，调用回调函数进行处理");
            if(_cb_message) _cb_message(_conn, msg);
        }
    }
protected:
    const int maxDataSize = (1<<16);
    BaseProtocol::Ptr _protocol;
    BaseConnection::Ptr _conn;
    muduo::CountDownLatch _downlatch;
    muduo::net::EventLoopThread _loopthread;
    muduo::net::EventLoop* _baseloop;
    muduo::net::TcpClient _client;
};
class ClientFactory{
public:
    template<typename ...Args>
    static BaseClient::Ptr Create(Args&& ...args){
        return std::make_shared<MuduoClient>(std::forward<Args>(args)...); 
    }
};
}