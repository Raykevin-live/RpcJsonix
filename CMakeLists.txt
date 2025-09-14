cmake_minimum_required(VERSION 3.11)
project(RpcJsonix)

set(CMAKE_VERSION 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_COMMANDS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.vscode/compile_commands.json")

find_package(jsoncpp REQUIRED)
if(NOT jsoncpp_FOUND)
    message(FATAL_ERROR "jsoncpp 库未找到，请安装 jsoncpp 开发包")
endif()

find_package(Boost REQUIRED COMPONENTS system thread)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost 库未找到，请安装 Boost 开发包")
endif()

# find_package(Muduo REQUIRED)
# if(NOT Muduo_FOUND)
#     message(FATAL_ERROR "Muduo 库未找到，请安装 Muduo 开发包")
# endif()

# muduo库
# 1. 手动指定 Muduo 头文件路径（源码中的 include 目录）
set(MUDUO_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/include)  # 替换为Muduo 源码根目录
include_directories(${MUDUO_INCLUDE_DIR})
# 2. 手动指定 Muduo 库文件路径（编译后的 lib 目录）
set(MUDUO_LIB_DIR ${CMAKE_SOURCE_DIR}/libs/lib)  # 替换为你的 Muduo 编译输出 lib 目录
link_directories(${MUDUO_LIB_DIR})


add_subdirectory(demo)

