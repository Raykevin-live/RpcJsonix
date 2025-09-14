#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace muduo;

class DictServer
{
public:
    DictServer(uint16_t port)
        :_server(&_baseloop, 
            net::InetAddress("127.0.0.1", port), 
            "DictServer", 
            net::TcpServer::kNoReusePort/*开启地址重用*/
        )
    {
        _server.setConnectionCallback(std::bind(&DictServer::OnConnection, this, std::placeholders::_1)); //参数绑定
        _server.setMessageCallback(std::bind(&DictServer::OnMessage, this, 
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3));
    }
    void Start()
    {
        // 顺序不能换
        _server.start(); //先开始监听
        _baseloop.loop(); //后开始死循环
    }
private:
    void OnConnection(const net::TcpConnectionPtr& connect)
    {
        if(connect->connected())
        {
            std::cout<<"连接建立!"<<std::endl;
        }
        else
        {
            std::cout<<"连接断开!"<<std::endl;
        }
    }
    void OnMessage(const net::TcpConnectionPtr& connect, net::Buffer* buffer, Timestamp time)
    {
        static std::unordered_map<std::string, std::string> dict_map = {
            {"hello", "你好"},
            {"Hard", "硬"},
            {"Big", "大"},
            {"Small", "小"},
            {"Large", "大"},
            {"Little", "小"},
            {"Long", "长"},
            {"Short", "短"},
            {"Tall", "高"},
            {"Short", "矮"},
            {"Fat", "胖"},
            {"Thin", "瘦"},
            {"Heavy", "重"},
            {"Light", "轻"},
        };
        std::string msg = buffer->retrieveAllAsString();
        std::string res;
        auto it = dict_map.find(msg);
        if(it != dict_map.end()) 
        {
            res = it->second;
        }
        else{
            res = "未知单词";
        }
        connect->send(res);
    }
private:
    net::EventLoop _baseloop;
    net::TcpServer _server;
};


int main()
{
    DictServer server(9090);
    server.Start();

    return 0;
}