#include "Target.h"
#include "base/LogStream.h"
#include "StringUtils.h"

using namespace tmms::base;
Target::Target(const std::string &s)
{
    auto list = StringUtils::SplitString(s,"/");
    if(list.size() == 3)
    {
        domain_name = list[0];
        app_name = list[1];
        stream_name = list[2];
    }
}
Target::Target(const std::string &d,const std::string &a)
:domain_name(d),app_name(a)
{

}
bool Target::ParseTarget(Json::Value &root)
{
    Json::Value &proObj = root["protocol"];
    if(!proObj.isNull())
    {
        protocol = std::move(proObj.asString());
    }

    Json::Value &urlObj = root["url"];
    if(!urlObj.isNull())
    {
        url = std::move(urlObj.asString());
    }    
    Json::Value &paramObj = root["param"];
    if(!paramObj.isNull())
    {
        param = std::move(paramObj.asString());
    }  
    Json::Value &hostObj = root["host"];
    if(!hostObj.isNull())
    {
        remote_host = std::move(hostObj.asString());
        auto pos = remote_host.find_first_of(":");
        if(pos != std::string::npos)
        {
            remote_port = std::atoi(remote_host.substr(pos+1).c_str());
            remote_host = remote_host.substr(0,pos);
        }
        else
        {
            if(protocol == "rtmp")
            {
                remote_port = 1935;
            }
            else if(protocol == "http")
            {
                remote_port = 80;
            }
            else if(protocol == "pav")
            {
                remote_port = 22000;
            }
        }
    }  
    else 
    {
        ParseTargetUrl(url);
    }

    Json::Value &portObj = root["port"];
    if(!portObj.isNull())
    {
        remote_port = portObj.asUInt();
    }   

    Json::Value &intervalObj = root["interval"];
    if(!intervalObj.isNull())
    {
        interval = intervalObj.asUInt();
    }   

    Json::Value &max_retryObj = root["max_retry"];
    if(!max_retryObj.isNull())
    {
        max_retry = max_retryObj.asUInt();
    }  
    LOG_TRACE << "target:" << session_name 
            << " protocol:" << protocol
            << " url:" << url
            << " param:" << param
            << " host:" << remote_host
            << " port:" << remote_port
            << " max retry:" << max_retry
            << " interval:" << interval;
    return true;
}
void Target::ParseTargetUrl(const std::string &url)
{
    if(protocol == "http")
    {
        auto pos = url.find_first_of("/",7);
        if(pos != std::string::npos)
        {
            auto host = url.substr(7,pos);
            auto pos1 = remote_host.find_first_of(":");
            if(pos1 != std::string::npos)
            {
                remote_port = std::atoi(remote_host.substr(pos1+1).c_str());
                remote_host = remote_host.substr(0,pos1);
            }
            else 
            {
                remote_host = host;
                remote_port = 80;
            }
        }
        else 
        {
            remote_host = url.substr(7);
            remote_port = 80;
        }
    }
}