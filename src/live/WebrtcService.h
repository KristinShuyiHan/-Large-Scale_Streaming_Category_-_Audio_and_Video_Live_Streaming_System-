#pragma once

#include "mmedia/webrtc/WebrtcHandler.h"
#include "mmedia/http/HttpHandler.h"
#include "live/user/WebrtcPlayerUser.h"
#include "base/Singleton.h"
#include <cstdint>
#include <string>

namespace tmms
{
    namespace live
    {
        using namespace mm;
        using WebrtcPlayerUserPtr = std::shared_ptr<WebrtcPlayerUser>;
        class WebrtcService:public WebrtcHandler,public HttpHandler
        {
        public:
            WebrtcService() = default;
            ~WebrtcService() = default;
            void OnStun(const network::UdpSocketPtr &socket,const network::InetAddress &addr,network::MsgBuffer &buf) override;
            void OnDtls(const network::UdpSocketPtr &socket,const network::InetAddress &addr,network::MsgBuffer &buf) override;
            void OnRtp(const network::UdpSocketPtr &socket,const network::InetAddress &addr,network::MsgBuffer &buf) override;
            void OnRtcp(const network::UdpSocketPtr &socket,const network::InetAddress &addr,network::MsgBuffer &buf) override;

            void OnRequest(const TcpConnectionPtr &conn,const HttpRequestPtr &req,const PacketPtr &packet) override;
            void OnSent(const TcpConnectionPtr &conn) override{}
            bool OnSentNextChunk(const TcpConnectionPtr &conn) override{return false;}  
            void OnNewConnection(const TcpConnectionPtr &conn) override{}
            void OnConnectionDestroy(const TcpConnectionPtr &conn) override{}
            void OnRecv(const TcpConnectionPtr &conn ,const PacketPtr &data) override{}
            void OnRecv(const TcpConnectionPtr &conn ,PacketPtr &&data) override{}
            void OnActive(const ConnectionPtr &conn) override{}    
            void Push2Players(); 
        private:
            static std::string GetSessionNameFromUrl(const std::string &url);
            std::mutex lock_;
            std::unordered_map<std::string,WebrtcPlayerUserPtr> name_users_;
            std::mutex users_lock_;
            std::unordered_map<std::string,WebrtcPlayerUserPtr> users_;
        };
        #define sWebrtcService tmms::base::Singleton<tmms::live::WebrtcService>::Instance()
    }
}