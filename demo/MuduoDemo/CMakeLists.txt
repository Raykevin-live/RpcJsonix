add_executable(muduo_server MuduoServer.cpp)
target_include_directories(muduo_server PRIVATE ${MUDUO_INCLUDE_DIR})
target_link_libraries(muduo_server PRIVATE muduo_net muduo_base pthread)

add_executable(muduo_client MuduoClient.cpp)
target_include_directories(muduo_client PRIVATE ${MUDUO_INCLUDE_DIR})
target_link_libraries(muduo_client PRIVATE muduo_net muduo_base pthread)
