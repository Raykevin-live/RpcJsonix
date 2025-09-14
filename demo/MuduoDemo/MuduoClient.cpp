#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/net/EventLoopThread.h>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace muduo;

class DictClient
{
public:
    DictClient(const std::string& sip, uint16_t port)
        :_baseloop(_loopthread.startLoop())
        ,_downlatch(1) // 初始化计数器为1，为0时被唤醒
        ,_client(_baseloop, net::InetAddress(sip, port), "DictClient")
    {
        _client.setConnectionCallback(std::bind(&DictClient::OnConnection, this, std::placeholders::_1)); //参数绑定
        _client.setMessageCallback(std::bind(&DictClient::OnMessage, this, 
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3));
        // 连接服务器
        _client.connect();
        _downlatch.wait(); //等待
        // _baseloop->loop(); // 开始事件循环监控--内部是个死循环，对于客户端来说不能直接使用，因为一旦开始循环，就走不下去了
    }

    bool Send(const std::string& msg)
    {
        if(_conn->connected() == false)
        {
            std::cout<<"连接断开，发送失败!"<<std::endl;
            return false;
        }
        _conn->send(msg);
        return true;
    }
private:
    void OnConnection(const net::TcpConnectionPtr& connect)
    {
       if(connect->connected())
        {
            std::cout<<"连接建立!"<<std::endl;
            _downlatch.countDown(); //计数--，为0时唤醒阻塞
            _conn = connect;
        }
        else
        {
            std::cout<<"连接断开!"<<std::endl;
            _conn.reset();
        }
    }
    void OnMessage(const net::TcpConnectionPtr& connect, net::Buffer* buffer, Timestamp time)
    {
       std::string res = buffer->retrieveAllAsString();
       std::cout<<res<<std::endl;
    }
private:
    net::TcpConnectionPtr _conn;
    CountDownLatch _downlatch;
    net::EventLoopThread _loopthread;
    net::EventLoop* _baseloop;
    net::TcpClient _client;
};

int main()
{
    DictClient client("127.0.0.1", 9090);
    while(1)
    {
        std::string msg;
        std::cin>>msg;
        client.Send(msg);
    }
    return 0;
}