#pragma once
#include "../common/Net.hpp"
#include "../common/Message.hpp"

using namespace base;

namespace server{
// Rpc请求参数类型枚举
enum class ValueType{
    INTERGRAL = 0,
    NUMERIC,
    STRING,
    ARRAY,
    OBJECT,
    BOOL
};
class ServiceDiscribe{
public:
    using Ptr = std::shared_ptr<ServiceDiscribe>;
    using ServiceCallback = std::function<void(const Json::Value&, Json::Value&)>;
    using ParamsDescribe = std::pair<std::string, ValueType>;

    ServiceDiscribe(std::string&& name, std::vector<ParamsDescribe>&& desc, 
        ValueType type, ServiceCallback&& handler) noexcept
        :_method_name(std::move(name))
        ,_callback(std::move(handler))
        ,_params_desc(std::move(desc))
        ,_return_type(type)
        {}
    bool ParamCheck(const Json::Value& params){
        // 判断所描述的参数类型是否存在，类型是否一致
        for(const auto& desc : _params_desc){
            LOG_DEBUG("参数: {}, 参数类型: {}", desc.first, (int)desc.second);
            if(params.isMember(desc.first) == false){
                LOG_ERROR("参数字段完整性校验失败! '{}' 字段缺失", desc.first);
                return false;
            }
            if(Check(desc.second, params[desc.first]) == false){
                LOG_ERROR("参数'{}' 类型校验失败!", desc.first);
                return false;
            }
        }
        return true;
    }
    const std::string& MethodName(){return _method_name;}
    bool ReturnTypeCheck(const Json::Value& val){
        return Check(_return_type, val);
    }
    bool Call(const Json::Value& params, Json::Value& result){
        _callback(params, result);
        if(ReturnTypeCheck(result) == false){
            LOG_ERROR("回调处理函数中的响应信息校验失败!");
            return false;
        }
        return true;
    }
private:
    bool Check(ValueType type, const Json::Value& val){
        switch (type)
        {
        case ValueType::BOOL: return val.isBool();
        case ValueType::INTERGRAL: return val.isIntegral();
        case ValueType::NUMERIC: return val.isNumeric();
        case ValueType::STRING: return val.isString();
        case ValueType::ARRAY: return val.isArray();
        case ValueType::OBJECT: return val.isObject();
        default:
            LOG_ERROR("不存在的参数类型");
            return false;
        }
    }
private:
    std::string _method_name; ///< 方法名称
    ServiceCallback _callback; ///< 实际的业务回调函数
    std::vector<ParamsDescribe> _params_desc; ///< 参数字段格式描述
    ValueType _return_type; ///< 返回值类型
};

/**
 * @brief: 构建者模式工厂
 */
class ServiceDiscribeFactory{
    using ServiceCallback = ServiceDiscribe::ServiceCallback;
    using ParamsDescribe = ServiceDiscribe::ParamsDescribe;
public:
    static ServiceDiscribe::Ptr Create();
    void SetReturnType(ValueType type){
        _return_type = type;
    }
    void SetMethodName(std::string method_name){
        _method_name = method_name;
    }
    void SetParamsDesc(const std::string& param_name, ValueType type){
        _params_desc.emplace_back(param_name, type);
    }
    void SetCallback(const ServiceCallback& cb){
        _callback = cb;
    }
    ServiceDiscribe::Ptr Build(){
        return std::make_shared<ServiceDiscribe>(std::move(_method_name), std::move(_params_desc), 
                                                _return_type, std::move(_callback));
    }
private:
    std::string _method_name;
    ServiceCallback _callback; ///< 实际的业务回调函数
    std::vector<ParamsDescribe> _params_desc; ///< 参数字段格式描述
    ValueType _return_type; ///< 返回值类型
};

// 服务的增删改查类: 因为服务需要加锁管理，所以需要分开管理更好
class ServiceManger{
public:
    using Ptr = std::shared_ptr<ServiceManger>;
    void Insert(const ServiceDiscribe::Ptr& desc){
        std::unique_lock<std::mutex> lock(_mutex);
        _services.emplace(desc->MethodName(), desc);
    }
    ServiceDiscribe::Ptr Select(const std::string& method_name){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _services.find(method_name);
        if(it != _services.end()){
            return it->second;
        }
        return ServiceDiscribe::Ptr();
    }
    void Remove(const std::string& method_name){
        std::unique_lock<std::mutex> lock(_mutex);
        _services.erase(method_name);
    }
private:
    std::mutex _mutex;
    std::unordered_map<std::string, ServiceDiscribe::Ptr> _services; //服务管理容器
};

/**@brief: 【对外接口类】 
 * 
 */
class RpcRouter{
public:
    using Ptr = std::shared_ptr<RpcRouter>;

    RpcRouter():_service_manager(std::make_shared<ServiceManger>()){}

    // 注册到Dispatcher模块针对Rpc请求进行回调处理的业务函数
    void OnRpcRequest(const BaseConnection::Ptr& conn, RpcRequest::Ptr& request){
        //1. 查询客户端请求的方法描述--判断当前服务端是否能提供响应的服务
        auto service = _service_manager->Select(request->Method());
        if(service.get() == nullptr){
            LOG_ERROR("{} 服务未找到", request->Method());
            return Response(conn, request, Json::Value(), ResCode::RCODE_NOT_FOUND_SERVICE);
        }
        //2. 进行参数校验，确定能否提供服务
        if(service->ParamCheck(request->Params()) == false){
            LOG_ERROR("{} 服务参数校验失败", request->Method());
            return Response(conn, request, Json::Value(), ResCode::RCODE_INVAILED_PARAMS);
        }
        //3. 调用业务回调函数接口进行业务处理
        Json::Value result;
        bool ret = service->Call(request->Params(), result);
        if(ret == false){
            LOG_ERROR("{} 服务调用错误", request->Method());
            return Response(conn, request, Json::Value(), ResCode::RCODE_INTERNAL_ERROR);  
        }
        //4. 处理完毕得到结果，组织响应， 向客户端发送
        return Response(conn, request, result, ResCode::RCODE_OK);  
    }
    void RegisterMethod(const ServiceDiscribe::Ptr& service){
        return _service_manager->Insert(service);
    }
private:
    void Response(const BaseConnection::Ptr& conn, RpcRequest::Ptr& req, 
        const Json::Value& res, ResCode rcode){
        auto msg = MessageFactory::Create<RpcResponse>();
        msg->SetId(req->Rid());
        msg->SetMessType(MessType::RESPONSE_RPC);
        msg->SetRcode(rcode);
        msg->SetResult(res);
        conn->Send(msg);
    }
private:
    ServiceManger::Ptr _service_manager;
};   
}