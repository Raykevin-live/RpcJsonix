#pragma once
#include "Abstract.hpp"
#include "Fileds.hpp"
#include "JsonComm.hpp"

using namespace common;
namespace base{
class JsonMessage : public BaseMessage
{
public:
    using Ptr = std::shared_ptr<JsonMessage>;
    virtual std::string Serialize() override{
        std::string body;
        bool ret = JsonUtil::Serialize(_body, body);
        if(ret == false)
        {
            return std::string();
        }
        return body;
    }
    virtual bool Unserialize(const std::string& msg) override{
        return JsonUtil::UnSerialize(msg, _body);
    }
protected:
    Json::Value _body;
};

class JsonRequest : public JsonMessage
{
public:
    using Ptr = std::shared_ptr<JsonRequest>;
private:
};
class JsonResponse : public JsonMessage
{
public:
    using Ptr = std::shared_ptr<JsonResponse>;
    virtual bool Check()override{
        // 判断响应状态码是否存在，类型是否正确
        if(_body[common::KEY_RCODE].isNull() == true){
            LOG_ERROR("响应中没有响应状态码!");
            return false;
        }
        if(_body[common::KEY_RCODE].isIntegral() == false){
            LOG_ERROR("响应状态码类型错误!");
            return false;
        }
        return true;
    }
    common::ResCode Rcode(){
        return static_cast<common::ResCode>(_body[KEY_RCODE].asInt());
    }
    void SetRcode(common::ResCode rcode){
        _body[KEY_RCODE] = static_cast<int>(rcode);
    }
};

class RpcRequest : public JsonRequest
{
public:
    using Ptr = std::shared_ptr<RpcRequest>;
    virtual bool Check()override{
        //Rpc 请求中，包含请求方法名称--字符串，参数字段-对象
        if(_body[common::KEY_METHOD].isNull() == true ||
            _body[common::KEY_METHOD].isString() == false){
                LOG_ERROR("RPC请求中没有方法名称或者方法名称类型错误!");
                return false;
        }
        if(_body[common::KEY_PARAMS].isNull() == true ||
            _body[common::KEY_PARAMS].isObject() == false){
                LOG_ERROR("RPC请求中没有参数信息或者参数信息错误!");
                return false;
        }
        return true;
    }
    std::string Method(){
        return _body[KEY_METHOD].asString();
    }
    void SetMethod(const std::string& method_name){
        _body[common::KEY_METHOD] = method_name;
    }
    Json::Value Params(){
        return _body[common::KEY_PARAMS];
    }
    void SetParams(const Json::Value& params){
        _body[common::KEY_PARAMS] = params;
    }
};
class RpcResponse : public JsonResponse
{
public:
    using Ptr = std::shared_ptr<RpcResponse>;
    virtual bool Check() override{
        if(_body[KEY_RCODE].isNull() == true ||
            _body[KEY_RCODE].isIntegral() ==false)
        {
            LOG_DEBUG("响应中没有响应状态码，或状态码类型错误!");
            return false;
        }
        if(_body[KEY_RESULT].isNull() == true)
        {
            LOG_DEBUG("响应中没有Rpc调用结果，或结果类型错误!");
            return false;
        }
        return true;
    }
    Json::Value Result(){
        return _body[common::KEY_RESULT];
    }
    void SetResult(const Json::Value& result){
        _body[common::KEY_RESULT] = result;
    }
};

class TopicRequest : public JsonRequest
{
public:
    using Ptr = std::shared_ptr<TopicRequest>;
    virtual bool Check()override{
        //Rpc 请求中，包含请求方法名称--字符串，参数字段-对象
        if(_body[common::KEY_TOPIC_KEY].isNull() == true ||
            _body[common::KEY_TOPIC_KEY].isString() == false){
                LOG_ERROR("主题请求中没有方法名称或者方法名称类型错误!");
                return false;
        }
        if(_body[common::KEY_OPTYPE].isNull() == true ||
            _body[common::KEY_OPTYPE].isIntegral() == false){
                LOG_ERROR("主题请求中没有操作类型或者操作类型的类型错误!");
                return false;
        }
        if(_body[common::KEY_OPTYPE].asInt() == static_cast<int>(TopicOperType::TOPIC_PUBLISH) &&
            (_body[common::KEY_TOPIC_MSG].isNull() == true || 
            _body[common::KEY_TOPIC_MSG].isString() == false))
        {
            LOG_DEBUG("主题消息发布请求中没有消息内容字段或消息内容类型错误!");
            return false;
        }
        return true;
    }
    std::string TopicKey(){
        return _body[KEY_TOPIC_KEY].asString();
    }
    void SetTopicKey(const std::string& key){
        _body[KEY_TOPIC_KEY] = key;
    }
    common::TopicOperType TopicOperType(){
        return static_cast<common::TopicOperType>(_body[KEY_OPTYPE].asInt());
    }
    void SetTopicOperType(const common::TopicOperType& key){
        _body[KEY_OPTYPE] = static_cast<int>(key);
    }
    std::string TopicMeassage(){
        return _body[KEY_TOPIC_MSG].asString();
    }
    void SetTopicMeassage(const std::string& msg){
        _body[KEY_TOPIC_MSG] = msg;
    }
};
class TopicResponse : public JsonResponse
{
public:
    using Ptr = std::shared_ptr<TopicResponse>;
};

typedef std::pair<std::string, int16_t> Address;
class ServiceRequest : public JsonRequest
{
public:
    using Ptr = std::shared_ptr<ServiceRequest>;
    virtual bool Check()override{
        //Rpc 请求中，包含请求方法名称--字符串，参数字段-对象
        if(_body[KEY_METHOD].isNull() == true ||
            _body[KEY_METHOD].isString() == false){
                LOG_ERROR("服务请求中没有方法名称或者方法名称类型错误!");
                return false;
        }
        if(_body[KEY_OPTYPE].isNull() == true ||
            _body[KEY_OPTYPE].isIntegral() == false){
                LOG_ERROR("服务请求中没有操作类型或者操作类型的类型错误!");
                return false;
        }
        // 服务发现的主机为空
        if(_body[KEY_OPTYPE].asInt() != static_cast<int>(ServiceOperType::SERVICE_DISCOVERY) && 
            (_body[KEY_HOST].isNull() ==true ||
            _body[KEY_HOST].isObject() == false ||
            _body[KEY_HOST][KEY_HOST_IP].isNull() == true ||
            _body[KEY_HOST][KEY_HOST_IP].isString() == false ||
            _body[KEY_HOST][KEY_HOST_PORT].isNull() == true ||
            _body[KEY_HOST][KEY_HOST_PORT].isIntegral() == false)
        )
        {
            LOG_DEBUG("服务请求中主机地址错误!");
            return false;
        }
        return true;
    }
    std::string Method(){
        return _body[KEY_METHOD].asString();
    }
    void SetMethod(const std::string& method){
        _body[KEY_METHOD] = method;  
    }
    common::ServiceOperType ServiceOperType(){
        return static_cast<common::ServiceOperType>(_body[KEY_OPTYPE].asInt());
    }
    void SetServiceOperType(common::ServiceOperType optype){
        _body[KEY_OPTYPE] = static_cast<int>(optype);
    }
    Address HostMeassage(){
        Address addr;
        addr.first = _body[KEY_HOST][KEY_HOST_IP].asString();
        addr.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
        return addr;
    }
    void SetHostMeassage(const Address& addr){
        Json::Value val;
        val[KEY_HOST_IP] = addr.first;
        val[KEY_HOST_PORT] = addr.second;
        _body[KEY_HOST] = val;
    }
};

class ServiceResponse : public JsonResponse
{
public:
    using Ptr = std::shared_ptr<ServiceResponse>;
    virtual bool Check() override{
        if(_body[KEY_RCODE].isNull() == true ||
            _body[KEY_RCODE].isIntegral() ==false)
        {
            LOG_DEBUG("响应中没有响应状态码，或状态码类型错误!");
            return false;
        }
        if(_body[KEY_OPTYPE].isNull() == true ||
            _body[KEY_OPTYPE].isIntegral() ==false)
        {
            LOG_DEBUG("响应中没有操作类型，或操作类型错误!");
            return false;
        }
        // 额外判断是不是服务发现
        if(_body[KEY_OPTYPE].asInt() == static_cast<int>(ServiceOperType::SERVICE_DISCOVERY) &&
            (_body[KEY_METHOD].isNull() ==true ||
            _body[KEY_METHOD].isString() == false ||
            _body[KEY_HOST].isNull() == true ||
            _body[KEY_HOST].isArray() == false)
        )
        {
            LOG_DEBUG("服务发现响应中信息字段错误!");
            return false;
        }
        return true;
    }
    std::string Method(){
        return _body[KEY_METHOD].asString();
    }
   
    void SetMethod(const std::string& method){
        _body[KEY_METHOD] = method;
    }
    void SetHosts(const std::vector<Address>& addrs){
        for(const auto& addr : addrs){
            Json::Value val;
            val[KEY_HOST_IP] = addr.first;
            val[KEY_HOST_PORT] = addr.second;
            _body[KEY_HOST].append(val);
        }
    }
    std::vector<Address> Hosts(){
        std::vector<Address> addrs;
        int size = _body[KEY_HOST].size();
        for(int i=0; i<size ; i++){
            Address addr;
            addr.first = _body[KEY_HOST][i][KEY_HOST_IP].asString();
            addr.second = _body[KEY_HOST][i][KEY_HOST_PORT].asInt();
            addrs.emplace_back(addr);
        }
        return addrs;
    }
    common::ServiceOperType ServiceOperType(){
        return static_cast<common::ServiceOperType>(_body[KEY_OPTYPE].asInt());
    }
    void SetServiceOperType(common::ServiceOperType optype){
        _body[KEY_OPTYPE] = static_cast<int>(optype);
    }
};
/* 工厂模式生产对象 */
class MessageFactory{
public:
    static BaseMessage::Ptr Create(const MessType& mtype){
        switch (mtype)
        {
        case MessType::REQUEST_RPC : 
            return std::make_shared<RpcRequest>();
        case MessType::RESPONSE_RPC : 
            return std::make_shared<RpcResponse>();
        case MessType::REQUEST_TOPIC : 
            return std::make_shared<TopicRequest>();
        case MessType::RESPONSE_TOPIC : 
            return std::make_shared<TopicResponse>();
        case MessType::REQUEST_SERVICE : 
            return std::make_shared<ServiceRequest>();
        case MessType::RESPONSE_SERVICE : 
            return std::make_shared<ServiceResponse>();
        default:
            return BaseMessage::Ptr();
        }
    }
    template<typename T, typename ...Args>
    static std::shared_ptr<T> Create(Args&& ...args){
        return std::make_shared<T>(std::forward(args)...);
    }
};
}
