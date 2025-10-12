#pragma once
#include <string_view>
#include <string>
#include <unordered_map>
/*
    消息类型字段
*/
namespace common {
/* 请求字段 */
inline const std::string KEY_METHOD = "method";
inline const std::string KEY_PARAMS = "parameters";
inline const std::string KEY_TOPIC_KEY = "topic_key";
inline const std::string KEY_TOPIC_MSG = "topic_message";
inline const std::string KEY_OPTYPE = "optype";
inline const std::string KEY_HOST = "host";
inline const std::string KEY_HOST_IP = "host_ip";
inline const std::string KEY_HOST_PORT = "host_port";
inline const std::string KEY_RCODE = "rcode";
inline const std::string KEY_RESULT = "result";
/* 消息类型 */
enum class MessType{
    REQUEST_RPC = 0, ///< Rpc请求
    RESPONSE_RPC, ///< Rpc响应
    REQUEST_TOPIC, ///< 主题请求
    RESPONSE_TOPIC, ///< 主题响应
    REQUEST_SERVICE, ///< 服务请求
    RESPONSE_SERVICE, ///< 服务响应
};

/*响应码类型 */
enum class ResCode{
    RCODE_OK = 0,
    RCODE_RARSE_FAILED, ///< 解析失败
    RCODE_INVALID_MSG, ///< 无效码
    RCODE_DISCONNECTED, ///< 未连接
    RCODE_INVAILED_PARAMS, ///< 无效参数
    RCODE_NOT_FOUND_SERVICE, ///< 未找到服务
    RCODE_INVALID_OPTYPE, ///< 无效请求类型
    RCODE_NOT_FOUND_TOPIC, ///< 未找到主题
    RCODE_INTERNAL_ERROR, ///< 内部错误
};
static std::string_view GetErrorReason(ResCode code)
{
    static std::unordered_map<ResCode, std::string_view> err_map = {
        {ResCode::RCODE_OK, "Success"},
        {ResCode::RCODE_RARSE_FAILED, "Prase error!"},
        {ResCode::RCODE_INVALID_MSG, "Invaild message"},
        {ResCode::RCODE_DISCONNECTED, "Connection had disconnected!"},
        {ResCode::RCODE_INVAILED_PARAMS, "Invaild Rpc parameters"},
        {ResCode::RCODE_NOT_FOUND_SERVICE, "Not find service"},
        {ResCode::RCODE_INVALID_OPTYPE, "Invaild opertor type"},
        {ResCode::RCODE_NOT_FOUND_TOPIC, "Not found right topic"},
        {ResCode::RCODE_INTERNAL_ERROR, "internal error"},
    };

    if(err_map.contains(code)) return "Invaild error rcode";
    return err_map[code];
}

/* Rpc 请求类型 */
enum class RequestType{
    REQUEST_SYNC = 0, ///< 同步
    REQUEST_ASYNC, ///< 异步
    REQUEST_CALLBACK ///< 回调
};

/* 主题操作类型 */
enum class TopicOperType{
    TOPIC_CREATE = 0, ///< 主题创建
    TOPIC_REMOVE, ///< 主题删除
    TOPIC_SUBSCRIBE, ///< 主题订阅
    TOPIC_CANCEL, ///< 主题取消
    TOPIC_PUBLISH ///< 主题发布
};

/* 服务操作类型 */
enum class ServiceOperType{
    SERVICE_REGISRY, ///< 服务注册
    SERVICE_DISCOVERY, ///< 服务发现
    SERVICE_ONLINE, ///< 服务上线
    SERVICE_OFFLINE, ///< 服务下线
    SERVICE_UNKNOW, 
};

} // namespace base