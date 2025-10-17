CFLAG= -std=c++20 -I ../../thirds/include/
LFLAG= -L../../thirds/lib/ -ljsoncpp -lfmt -lmuduo_net -lmuduo_base -pthread
DEGUG= #-g
all:PublishClient Server SubscribeClient

PublishClient:PublishClient.cpp
	g++ $(CFLAG) $^ -o $@ $(LFLAG) $(DEGUG)
Server:Server.cpp
	g++ $(CFLAG) $^ -o $@ $(LFLAG) $(DEGUG)
SubscribeClient:SubscribeClient.cpp
	g++ $(CFLAG) $^ -o $@ $(LFLAG) $(DEGUG)

.PHONY:clean
clean:
	rm -rf PublishClient Server SubscribeClient 
