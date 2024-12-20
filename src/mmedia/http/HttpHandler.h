#pragma once

#include "mmedia/base/MMediaHandler.h"

namespace tmms
{
    namespace mm
    {
        class HttpRequest;
        using HttpRequestPtr = std::shared_ptr<HttpRequest>;
        
        class HttpHandler:virtual public MMediaHandler
        {
        public:
            virtual void OnSent(const TcpConnectionPtr &conn) = 0;
            virtual bool OnSentNextChunk(const TcpConnectionPtr &conn) = 0;   
            virtual void OnRequest(const TcpConnectionPtr &conn,const HttpRequestPtr &req,const PacketPtr &packet) = 0; 
        }; 
    }
}