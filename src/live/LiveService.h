#pragma once

#include "network/net/Connection.h"
#include "network/TcpServer.h"
#include "network/net/EventLoopThreadPool.h"
#include "base/TaskMgr.h"
#include "base/Task.h"
#include "base/NonCopyable.h"
#include "base/Singleton.h"
#include "mmedia/rtmp/RtmpHandler.h"
#include "mmedia/http/HttpHandler.h"
#include "mmedia/webrtc/WebrtcServer.h"
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace tmms
{
    namespace live
    {
        using namespace tmms::network;
        using namespace tmms::base;
        using namespace tmms::mm;
        
        class Session;
        using SessionPtr = std::shared_ptr<Session>;

        class LiveService:public RtmpHandler,public HttpHandler
        {
        public:
            LiveService() = default;
            ~LiveService() = default;

            SessionPtr CreateSession(const std::string &session_name);
            SessionPtr FindSession(const std::string &session_name);
            bool CloseSession(const std::string &session_name); 
            void OnTimer(const TaskPtr &t);

            void OnNewConnection(const TcpConnectionPtr &conn) override;
            void OnConnectionDestroy(const TcpConnectionPtr &conn) override;
            void OnActive(const ConnectionPtr &conn) override;
            bool OnPlay(const TcpConnectionPtr &conn,const std::string &session_name, const std::string &param) override;
            bool OnPublish(const TcpConnectionPtr &conn,const std::string &session_name, const std::string &param) override;
            void OnRecv(const TcpConnectionPtr &conn ,PacketPtr &&data) override;
            void OnRecv(const TcpConnectionPtr &conn ,const PacketPtr &data) override{};
            void OnSent(const TcpConnectionPtr &conn) override;
            bool OnSentNextChunk(const TcpConnectionPtr &conn) override;   
            void OnRequest(const TcpConnectionPtr &conn,const HttpRequestPtr &req,const PacketPtr &packet) override;

            void Start();
            void Stop();
            EventLoop *GetNextLoop();
            std::shared_ptr<WebrtcServer> GetWebrtcServer()const 
            {
                return webrtc_server_;
            }
        private:
            EventLoopThreadPool * pool_{nullptr};
            std::vector<TcpServer*> servers_;
            std::mutex lock_;
            std::unordered_map<std::string,SessionPtr> sessions_;

            std::shared_ptr<WebrtcServer>  webrtc_server_;
        };
        #define sLiveService tmms::base::Singleton<tmms::live::LiveService>::Instance()
    }
}