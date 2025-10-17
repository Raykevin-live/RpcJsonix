#include "../../source/server/RpcServer.hpp"

int main(){
    auto server = std::make_shared<server::TopicServer>(7070);
    server->Start();

    return 0;
}