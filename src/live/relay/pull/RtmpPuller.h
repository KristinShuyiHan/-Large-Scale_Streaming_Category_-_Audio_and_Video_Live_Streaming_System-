#pragma once

#include "network/net/EventLoop.h"
#include "live/Session.h"
#include "live/relay/pull/Puller.h"
#include "base/Target.h"
#include "mmedia/rtmp/RtmpHandler.h"
#include "mmedia/rtmp/RtmpClient.h"

namespace tmms
{
    namespace live
    {
        using namespace tmms::mm;
        using namespace tmms::network;
        using namespace tmms::base;

        class RtmpPuller:public RtmpHandler,public Puller
        {
        public:
            RtmpPuller(EventLoop *loop, Session *s ,PullHandler *handler);
            ~RtmpPuller();
            bool Pull(const TargetPtr &target) override;
            void OnNewConnection(const TcpConnectionPtr &conn) override;
            void OnConnectionDestroy(const TcpConnectionPtr &conn)override;
            void OnRecv(const TcpConnectionPtr &conn ,PacketPtr &&data)override;
            bool OnPlay(const TcpConnectionPtr &conn,const std::string &session_name, const std::string &param) override;
            void OnRecv(const TcpConnectionPtr &conn ,const PacketPtr &data)override{}
            void OnActive(const ConnectionPtr &conn)override{}
        private:
            TargetPtr target_;
            RtmpClient * rtmp_client_{nullptr};
        };
    }
}