#pragma once
#include <string>
#include <jsoncpp/json/json.h>
#include "Logging.hpp"

namespace common{
class JsonUtil{
public:
    static bool Serialize(const Json::Value& value, std::string& body)
    {
        Json::StreamWriterBuilder Wbuilder;
        Wbuilder["emitUTF8"] = true;  // 关键配置：输出UTF-8编码的中文
        std::unique_ptr<Json::StreamWriter> Swriter(Wbuilder.newStreamWriter());

        std::stringstream ss;
        int ret = Swriter->write(value, &ss);
        if(ret != 0) 
        {
            LOG_ERROR("json serialize failed!");
            return false;
        }
        body = ss.str();
        return true;
    }

    static bool UnSerialize(const std::string& body, Json::Value& rvalue)
    {
        Json::CharReaderBuilder CBuilder;
        CBuilder["emitUTF8"] = true;
        std::unique_ptr<Json::CharReader> Creader(CBuilder.newCharReader());

        std::string error;
        int ret = Creader->parse(body.c_str(), body.c_str()+body.size(), &rvalue, &error);
        if(!ret) 
        {
            LOG_ERROR("json unserialize failed: {}", error);
            return false;
        }
        return true;
    }
}; //class JsonUtil
} //namespace detail