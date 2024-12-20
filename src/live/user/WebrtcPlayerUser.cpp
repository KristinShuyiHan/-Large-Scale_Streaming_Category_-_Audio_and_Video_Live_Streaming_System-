#include "WebrtcPlayerUser.h"
#include "live/base/LiveLog.h"
#include "base/Config.h"
#include "live/Session.h"
#include "network/net/UdpSocket.h"
#include "live/LiveService.h"
#include "live/Stream.h"
#include "base/TTime.h"
#include "mmedia/rtp/SenderReport.h"
#include "mmedia/rtp/Rtpfb.h"
#include <sys/time.h>
#include <random>

using namespace tmms::live;

WebrtcPlayerUser::WebrtcPlayerUser(const ConnectionPtr &ptr,const StreamPtr &stream,const SessionPtr &s)
:PlayerUser(ptr,stream,s),dtls_(this),video_queue_(500),audio_queue_(500)
{
    local_ufrag_ = GetUFrag(8);
    local_passwd_ = GetUFrag(32);
    uint32_t audio_ssrc = GetSsrc(10);
    uint32_t video_ssrc = audio_ssrc + 1;

    sdp_.SetLocalUFrag(local_ufrag_);
    sdp_.SetLocalPasswd(local_passwd_);
    sdp_.SetAudioSsrc(audio_ssrc);
    sdp_.SetVideoSsrc(video_ssrc);
    dtls_.Init();

    auto config = sConfigMgr->GetConfig();
    if(config)
    {
        auto serverinfo = config->GetServiceInfo("webrtc","udp");
        if(serverinfo)
        {
            sdp_.SetServerAddr(serverinfo->addr);
            sdp_.SetServerPort(serverinfo->port);
        }
    }
    sdp_.SetStreamName(s->SessionName());
}

bool WebrtcPlayerUser::PostFrames()
{
   if(!stream_->Ready()||!stream_->HasMedia()||!dtls_done_)
    {
        Deactive();
        return false;
    }
    
    stream_->GetFrames(std::dynamic_pointer_cast<PlayerUser>(shared_from_this()));
    meta_.reset();
    std::list<PacketPtr> rtp_pkts;
    if(audio_header_)
    {
        rtp_muxer_.EncodeAudio(audio_header_,rtp_pkts,0);
        audio_header_.reset();
    }    
    
    if(video_header_)
    {
        rtp_muxer_.EncodeVideo(video_header_,rtp_pkts,0);
        video_header_.reset();
    } 
    
    if(!out_frames_.empty())  
    {
        for(auto &p:out_frames_)
        {
            if(p->IsVideo()&&p->IsKeyFrame())
            {
                got_key_frame_ = true;
            }
            if(!got_key_frame_)
            {
                continue;
            }
            if(p->IsAudio())
            {
                rtp_muxer_.EncodeAudio(p,rtp_pkts,p->TimeStamp());
            }
            else if(p->IsVideo())
            {
                rtp_muxer_.EncodeVideo(p,rtp_pkts,p->TimeStamp());
            }
        }
        out_frames_.clear();
    }  
    if(!rtp_pkts.empty())
    {
        std::list<PacketPtr> result;
        for(auto &p:rtp_pkts)
        {
            bool is_video = true;
            if(p->IsAudio())
            {
                is_video = false;
                AddAudio(p);
            }
            else
            {
                AddVideo(p);
            }
            auto np = srtp_.RtpProtect(p);
            if(np)
            {
                if(is_video)
                {
                    video_out_bytes_ += np->PacketSize();
                    ++ video_out_pkts_count_;
                }
                else
                {
                    audio_out_bytes_ += np->PacketSize();
                    ++ audio_out_pkts_count_;
                }
                np->SetExt(addr_);
                result.emplace_back(np);
            }
        }
        auto server = sLiveService->GetWebrtcServer();
        server->SendPacket(result);
        CheckSR();
    }
    else
    {
        Deactive();
    }
    return true;
}
UserType WebrtcPlayerUser::GetUserType() const
{
    return UserType::kUserTypePlayerWebRTC;
}
std::string WebrtcPlayerUser::GetUFrag(int size)
{
    static std::string table = "1234567890abcdefgihjklmnopqrstuvwsyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string frag;

    static std::mt19937 mt{std::random_device{}()};
    static std::uniform_int_distribution<> rand(0,table.size());

    frag.resize(size);
    for(int i=0;i<size;i++)
    {
        frag[i] = table[(rand(mt)%table.size())];
    }

    return frag;
}
uint32_t WebrtcPlayerUser::GetSsrc(int size)
{
    static std::mt19937 mt{std::random_device{}()};
    static std::uniform_int_distribution<> rand(10000000,99999999);

    return rand(mt);
}
bool WebrtcPlayerUser::ProcessOfferSdp(const std::string &sdp)
{
    return sdp_.Decode(sdp);
}
const std::string &WebrtcPlayerUser::LocalUFrag() const
{
    return sdp_.GetLocalUFrag();
}
const std::string &WebrtcPlayerUser::LocalPasswd() const
{
    return sdp_.GetLocalPasswd();
}
const std::string &WebrtcPlayerUser::RemoteUFrag() const
{
    return sdp_.GetRemoteUFrag();
}
std::string WebrtcPlayerUser::BuildAnswerSdp()
{
    sdp_.SetFingerprint(dtls_.Fingerprint());
    return sdp_.Encode();
}

void WebrtcPlayerUser::SetConnection(const ConnectionPtr &conn)
{
    User::SetConnection(conn);
}

void WebrtcPlayerUser::OnDtlsSend(const char *data,size_t size, Dtls *dtls) 
{
    LIVE_DEBUG << "dtls send size:" << size;
    PacketPtr packet = Packet::NewPacket(size);
    memcpy(packet->Data(),data,size);
    packet->SetPacketSize(size);

    packet->SetExt(addr_);
    auto server = sLiveService->GetWebrtcServer();
    server->SendPacket(packet);
}
void WebrtcPlayerUser::OnDtlsHandshakeDone(Dtls *dtls)
{
    LIVE_DEBUG << "dtls handshake done.";
    dtls_done_ = true;
    srtp_.Init(dtls_.RecvKey(),dtls_.SendKey());
    rtp_muxer_.Init(sdp_.GetVideoPayloadType(),
                    sdp_.GetAudioPayloadType(),
                    sdp_.VideoSsrc(),
                    sdp_.AudioSsrc());
}
void WebrtcPlayerUser::OnDtlsRecv(const char *buf,size_t size)
{
    dtls_.OnRecv(buf,size);
}

void WebrtcPlayerUser::SendSR(bool is_video)
{
    uint64_t now = base::TTime::NowMS();
    uint64_t elapse = 0;
    if(is_video)
    {
        elapse = now - video_sr_timestamp_;
    }
    else 
    {
        elapse = now - audio_sr_timestamp_;
    }

    if(elapse < 3000)
    {
        return ;
    }

    SenderReport sr;
    struct timeval tv;    
    gettimeofday(&tv, NULL);
    uint64_t ntp = tv.tv_sec*1000000+tv.tv_usec;
    sr.SetNtpTimestamp(ntp);

    if(is_video)
    {
        sr.SetSsrc(rtp_muxer_.VideoSsrc());
        sr.SetRtpTimestamp(rtp_muxer_.VideoTimestamp());
        sr.SetSentBytes(video_out_bytes_);
        sr.SetSentPacketCount(video_out_pkts_count_);
    }
    else
    {
        sr.SetSsrc(rtp_muxer_.AudioSsrc());
        sr.SetRtpTimestamp(rtp_muxer_.AudioTimestamp());
        sr.SetSentBytes(audio_out_bytes_);
        sr.SetSentPacketCount(audio_out_pkts_count_);        
    }

    auto packet = sr.Encode();
    if(packet)
    {
        auto np = srtp_.RtcpProtect(packet);
        if(np)
        {
            np->SetExt(addr_);
            auto server = sLiveService->GetWebrtcServer();
            server->SendPacket(np);
        }
    }

    if(is_video)
    {
        video_sr_timestamp_ = now;
    }
    else 
    {
        audio_sr_timestamp_ = now;
    }
}
void WebrtcPlayerUser::CheckSR()
{
    SendSR(true);
    SendSR(false);
}

void WebrtcPlayerUser::AddVideo(const PacketPtr &pkt)
{
    std::lock_guard<std::mutex> lk(queue_lock_);
    int index = pkt->Index()%video_queue_.size();
    video_queue_[index] = pkt;
}
void WebrtcPlayerUser::AddAudio(const PacketPtr &pkt)
{
    std::lock_guard<std::mutex> lk(queue_lock_);
    int index = pkt->Index()%audio_queue_.size();
    audio_queue_[index] = pkt;
}
PacketPtr WebrtcPlayerUser::GetVideo(int idx)
{
    std::lock_guard<std::mutex> lk(queue_lock_);
    int index = idx%video_queue_.size();
    auto pkt = video_queue_[index];
    if(pkt&&pkt->Index()==idx)
    {
        return pkt;
    }
    return PacketPtr();
}
PacketPtr WebrtcPlayerUser::GetAudio(int idx)
{
    std::lock_guard<std::mutex> lk(queue_lock_);
    int index = idx%audio_queue_.size();
    auto pkt = audio_queue_[index];
    if(pkt&&pkt->Index()==idx)
    {
        return pkt;
    }
    return PacketPtr();
}
void WebrtcPlayerUser::ProcessRtpfb(const char *buf,size_t size)
{
    Rtpfb rtpfb;
    rtpfb.Decode(buf,size);
    
    uint32_t media_ssrc = rtpfb.MediaSsrc();
    bool is_video = (media_ssrc == rtp_muxer_.VideoSsrc());
    bool is_audio = (media_ssrc == rtp_muxer_.AudioSsrc());
    if(!is_audio&&!is_video)
    {
        return;
    }
    auto lost_sets = rtpfb.LostSeqs();
    std::list<PacketPtr> lost_list;
    for(auto &l:lost_sets)
    {
        if(is_audio)
        {
            auto pkt = GetAudio(l);
            if(pkt)
            {
                lost_list.emplace_back(pkt);
            }
        }
        else if(is_video)
        {
            auto pkt = GetVideo(l);
            if(pkt)
            {
                lost_list.emplace_back(pkt);
            }            
        }
    }
    if(!lost_list.empty())
    {
        std::list<PacketPtr> result;
        for(auto &p:lost_list)
        {
            auto np = srtp_.RtpProtect(p);
            if(np)
            {
                np->SetExt(addr_);
                result.emplace_back(np);
            }
        }
        if(!result.empty())
        {
            auto server = sLiveService->GetWebrtcServer();
            server->SendPacket(result);
        }
    }
}
void WebrtcPlayerUser::OnRtcp(const char *buf,size_t size)
{
    auto np = srtp_.SrtcpUnprotect(buf,size);
    if(!np)
    {
        return;
    }
    uint8_t pt = (uint8_t)buf[1];
    LIVE_DEBUG << "on rtcp size:" << size << " pt:" << std::to_string(pt);
    switch(pt)
    {
        case kRtcpPtRR:
        {
            break;
        }
        case kRtcpPtRtpfb:
        {
            ProcessRtpfb(np->Data(),np->PacketSize());
            break;
        }
        case kRtcpPtPsfb:
        {
            break;
        }
    }
}