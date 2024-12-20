#pragma once

#include "network/UdpServer.h"
#include "network/net/UdpSocket.h"
#include "network/net/EventLoop.h"
#include "WebrtcHandler.h"
#include "mmedia/base/AVTypes.h"
#include "mmedia/base/Packet.h"
#include <cstdint>
#include <list>

namespace tmms
{
    namespace mm
    {
        using namespace network;
        class WebrtcServer
        {
        public:
            WebrtcServer(EventLoop *loop,const InetAddress &server,WebrtcHandler *handler);
            ~WebrtcServer() = default;

            void Start();
            void SendPacket(const PacketPtr &packet);
            void SendPacket(std::list<PacketPtr> &list);

        private:
            void MessageCallback(const UdpSocketPtr &socket,const InetAddress &addr,MsgBuffer &buf);
            bool IsDtls(MsgBuffer &buf);
            bool IsStun(MsgBuffer &buf);
            bool IsRtp(MsgBuffer &buf);
            bool IsRtcp(MsgBuffer &buf);
            void OnSend();
            void WriteComplete(const UdpSocketPtr &socket);

            WebrtcHandler * handler_{nullptr};
            std::shared_ptr<UdpServer> udp_server_;
            std::list<PacketPtr> sending_;
            std::list<PacketPtr> out_waiting_;
            std::list<UdpBufferNodePtr> udp_outs_;
            bool b_sending_{false};
            std::mutex lock_;
        };
    }
}