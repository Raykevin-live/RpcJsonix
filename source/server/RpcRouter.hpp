#include "../common/Net.hpp"
#include "../common/Message.hpp"

using namespace base;

namespace server{
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
    using ServiceCallback = std::function<void(const Json::Value&, Json::Value)>;
    using ParamsDescribe = std::pair<std::string, ValueType>;
    bool ParamCheck(const Json::Value& params){
        // 判断所描述的参数类型是否存在，类型是否一致
    }
private:
    std::string _method_name; ///< 方法名称
    ServiceCallback _callback; ///< 实际的业务回调函数
    std::vector<ParamsDescribe> _params_desc; ///< 参数字段格式描述
    ValueType _return_type; ///< 返回值类型
};

class ServiceDiscribeFactory{
public:
    static ServiceDiscribe::Ptr Create();
};
// 服务的增删改查类
class ServiceManger{
public:
    using Ptr = std::shared_ptr<ServiceManger>;
    ServiceDiscribe::Ptr Create();
    ServiceDiscribe::Ptr Select();
    void Remove();
private:
    std::mutex _mutex;
    std::unordered_map<std::string, ServiceDiscribe::Ptr> _services;
};
class RpcRouter{
public:
    using Ptr = std::shared_ptr<RpcRouter>;

    // 注册到Dispatcher模块针对Rpc请求进行回调处理的业务函数
    void OnRpcRequest(const BaseConnection::Ptr& conn, RpcRequest::Ptr& request){
        //1. 查询客户端请求的方法描述--判断当前服务端是否能提供响应的服务

        //2. 进行参数校验，确定能否提供服务
        //3. 调用业务回调函数接口进行业务处理
        //4. 处理完毕得到结果，组织响应，
    }
    void RegisterMethod(const ServiceDiscribe::Ptr& service){

    }
private:
    ServiceManger::Ptr _service_manager;
};   
}