#include "WebrtcServer.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

WebrtcServer::WebrtcServer(EventLoop *loop,const InetAddress &server,WebrtcHandler *handler)
:handler_(handler)
{
    udp_server_ = std::make_shared<UdpServer>(loop,server);
}
void WebrtcServer::Start()
{
    udp_server_->SetRecvMsgCallback(std::bind(&WebrtcServer::MessageCallback,this,
                                    std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    udp_server_->SetWriteCompleteCallback(std::bind(&WebrtcServer::WriteComplete,this,
                                    std::placeholders::_1));
    udp_server_->Start();
    WEBRTC_DEBUG << "webrtc server start.";
}
void WebrtcServer::MessageCallback(const UdpSocketPtr &socket,const InetAddress &addr,MsgBuffer &buf)
{
    if(handler_)
    {
        if(IsStun(buf))
        {
            handler_->OnStun(socket,addr,buf);
        }
        else if(IsDtls(buf))
        {
            handler_->OnDtls(socket,addr,buf);
        }
        else if(IsRtp(buf))
        {
            handler_->OnRtp(socket,addr,buf);
        }
        else if(IsRtcp(buf))
        {
            handler_->OnRtcp(socket,addr,buf);
        }
    }
}
bool WebrtcServer::IsDtls(MsgBuffer &buf)
{
    const char *data = buf.Peek();
    return buf.ReadableBytes() >=13 && data[0] >=20&&data[0] <=63;
}
bool WebrtcServer::IsStun(MsgBuffer &buf)
{
    const char *data = buf.Peek();
    return buf.ReadableBytes() >=20 && data[0] >=0&&data[0] <=3;
}
bool WebrtcServer::IsRtp(MsgBuffer &buf)
{
    const char *data = buf.Peek();
    uint8_t pt = (uint8_t)data[1];
    return buf.ReadableBytes() >=12 && data[0] &0x80 &&!(pt>=192&&pt<=223);
}
bool WebrtcServer::IsRtcp(MsgBuffer &buf)
{
    const char *data = buf.Peek();
    uint8_t pt = (uint8_t)data[1];
    return buf.ReadableBytes() >=12 && data[0] &0x80 &&(pt>=192&&pt<=223);
}
void WebrtcServer::OnSend()
{
    if(b_sending_||out_waiting_.empty())
    {
        return;
    }
    udp_outs_.clear();
    for(auto &p:out_waiting_)
    {
        std::shared_ptr<struct sockaddr_in6> addr = p->Ext<struct sockaddr_in6>();
        if(addr)
        {
            UdpBufferNodePtr up = std::make_shared<UdpBufferNode>(p->Data(),
                                                                p->PacketSize(),
                                                                (struct sockaddr*)addr.get(),
                                                                sizeof(struct sockaddr_in6));
            udp_outs_.emplace_back(std::move(up));
            sending_.emplace_back(p);
        }
    }
    out_waiting_.clear();
    if(!udp_outs_.empty())
    {
        b_sending_ = true;
        auto socket = std::dynamic_pointer_cast<UdpSocket>(udp_server_);
        socket->Send(udp_outs_);
    }
}
void WebrtcServer::WriteComplete(const UdpSocketPtr &socket)
{
    std::lock_guard<std::mutex> lk(lock_);
    b_sending_ = false;
    sending_.clear();
    udp_outs_.clear();

    if(!out_waiting_.empty())
    {
        OnSend();
    }
}
void WebrtcServer::SendPacket(const PacketPtr &packet)
{
    std::lock_guard<std::mutex> lk(lock_);
    if(b_sending_)
    {
        out_waiting_.emplace_back(packet);
        return;
    }
    udp_outs_.clear();
    std::shared_ptr<struct sockaddr_in6> addr = packet->Ext<struct sockaddr_in6>();
    if(addr)
    {
        UdpBufferNodePtr up = std::make_shared<UdpBufferNode>(packet->Data(),
                                                            packet->PacketSize(),
                                                            (struct sockaddr*)addr.get(),
                                                            sizeof(struct sockaddr_in6));
        udp_outs_.emplace_back(std::move(up));
        sending_.emplace_back(packet);
    }
    if(!udp_outs_.empty())
    {
        b_sending_ = true;
        auto socket = std::dynamic_pointer_cast<UdpSocket>(udp_server_);
        socket->Send(udp_outs_);
    }    
}
void WebrtcServer::SendPacket(std::list<PacketPtr> &list)
{
    std::lock_guard<std::mutex> lk(lock_);
    if(b_sending_)
    {
        for(auto &p:list)
        {
            out_waiting_.emplace_back(p);
        }
        return;
    }
    udp_outs_.clear();
    for(auto &p:list)
    {
        std::shared_ptr<struct sockaddr_in6> addr = p->Ext<struct sockaddr_in6>();
        if(addr)
        {
            UdpBufferNodePtr up = std::make_shared<UdpBufferNode>(p->Data(),
                                                                p->PacketSize(),
                                                                (struct sockaddr*)addr.get(),
                                                                sizeof(struct sockaddr_in6));
            udp_outs_.emplace_back(std::move(up));
            sending_.emplace_back(p);
        }        
    }

    if(!udp_outs_.empty())
    {
        b_sending_ = true;
        auto socket = std::dynamic_pointer_cast<UdpSocket>(udp_server_);
        socket->Send(udp_outs_);
    } 
}