CFLAG= -std=c++20 -I ../../thirds/include/
LFLAG= -L../../thirds/lib/ -ljsoncpp -lfmt -lmuduo_net -lmuduo_base -pthread
# DEGUG= -g
all:ServerTest ClientTest RegistryServer

RegistryServer:RegistryServerTest.cpp
	g++ $(CFLAG) $^ -o $@ $(LFLAG) $(DEGUG)
ServerTest:RpcServerTest.cpp
	g++ $(CFLAG) $^ -o $@ $(LFLAG) $(DEGUG)
ClientTest:RpcClientTest.cpp
	g++ $(CFLAG) $^ -o $@ $(LFLAG) $(DEGUG)

.PHONY:clean
clean:
	rm -rf ServerTest ClientTest RegistryServer 
