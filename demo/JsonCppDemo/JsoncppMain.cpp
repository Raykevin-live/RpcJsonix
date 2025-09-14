#include <iostream>
#include <string>
#include <jsoncpp/json/json.h>

bool Serialize(const Json::Value& value, std::string& body)
{
    Json::StreamWriterBuilder Wbuilder;
    Wbuilder["emitUTF8"] = true;  // 关键配置：输出UTF-8编码的中文
    std::unique_ptr<Json::StreamWriter> Swriter(Wbuilder.newStreamWriter());

    std::stringstream ss;
    int ret = Swriter->write(value, &ss);
    if(ret != 0) return false;
    body = ss.str();
    return true;
}

bool UnSerialize(const std::string& body, Json::Value& rvalue)
{
    Json::CharReaderBuilder CBuilder;
    CBuilder["emitUTF8"] = true;
    std::unique_ptr<Json::CharReader> Creader(CBuilder.newCharReader());

    std::string error;
    int ret = Creader->parse(body.c_str(), body.c_str()+body.size(), &rvalue, &error);
    if(!ret) return false;
    return true;
}
int main()
{
    // 数据准备
    Json::Value value;
    value["性别"] = "男";
    value["年龄"] = 18;
    value["姓名"] = "小王";
    value["成绩"].append(88);
    value["成绩"].append(16);
    value["成绩"].append(100);

    std::string str;
    Serialize(value, str);
    std::cout<<str<<std::endl;

    Json::Value root;
    UnSerialize(str, root);

    std::cout<<"姓名: "<<root["姓名"].asString()<<std::endl;
    std::cout<<"性别: "<<root["性别"].asString()<<std::endl;
    std::cout<<"年龄: "<<root["年龄"].asInt()<<std::endl;
    int size = root["成绩"].size();
    std::cout<<"成绩";
    for(int i = 0; i<size; i++)
    {
        std::cout<<" "<<root["成绩"][i].asInt();
    }
    std::cout<<std::endl;
    return 0;
}