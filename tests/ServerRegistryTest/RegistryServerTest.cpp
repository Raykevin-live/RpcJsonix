#include "../../source/server/RpcServer.hpp"
#include "../../source/common/Logging.hpp"

using namespace base;
using namespace server;

int main(){
    RegistryServer reg_server(8080);
    reg_server.Start();

    return 0;
}