#include "Srtp.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

namespace 
{
    static PacketPtr null_packet;
}
bool Srtp::InitSrtpLibrary()
{
    WEBRTC_INFO << "srtp library version:" << srtp_get_version();
    auto ret = srtp_init();
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp init failed.";
        return false;
    }
    ret = srtp_install_event_handler(Srtp::OnSrtpEvent);
    return true;
}
bool Srtp::Init(const std::string &recv_key,const std::string &send_key)
{
    srtp_policy_t srtp_policy;
    memset(&srtp_policy,0,sizeof(srtp_policy_t));

    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&srtp_policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&srtp_policy.rtcp);

    srtp_policy.ssrc.value = 0;
    srtp_policy.window_size = 4096;
    srtp_policy.next = NULL;
    srtp_policy.allow_repeat_tx = 1;

    srtp_policy.ssrc.type = ssrc_any_inbound;
    srtp_policy.key = (unsigned char*)recv_key.c_str();
    auto ret = srtp_create(&recv_ctx_,&srtp_policy);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_create recv ctx failed.err=" << ret;
        return false;
    }

    srtp_policy.ssrc.type = ssrc_any_outbound;
    srtp_policy.key = (unsigned char*)send_key.c_str();
    ret = srtp_create(&send_ctx_,&srtp_policy);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_create send ctx failed.err=" << ret;
        return false;
    }
    return true;
}
PacketPtr Srtp::RtpProtect(PacketPtr &pkt)
{
    int32_t bytes = pkt->PacketSize();

    if(bytes + SRTP_MAX_TRAILER_LEN >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large,bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }

    std::memcpy(w_buffer_,pkt->Data(),bytes);
    auto ret = srtp_protect(send_ctx_,w_buffer_,&bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_protect failed.err:" << ret;
        return null_packet;
    }

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(),w_buffer_,bytes);
    npkt->SetPacketSize(bytes);
    return npkt;
}
PacketPtr Srtp::RtcpProtect( PacketPtr &pkt)
{
    int32_t bytes = pkt->PacketSize();

    if(bytes + SRTP_MAX_TRAILER_LEN >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large,bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }

    std::memcpy(w_buffer_,pkt->Data(),bytes);
    auto ret = srtp_protect_rtcp(send_ctx_,w_buffer_,&bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_protect_rtcp failed.err:" << ret;
        return null_packet;
    }

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(),w_buffer_,bytes);
    npkt->SetPacketSize(bytes);
    return npkt;
}
PacketPtr Srtp::SrtpUnprotect(PacketPtr &pkt)
{
    int32_t bytes = pkt->PacketSize();

    if(bytes >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large,bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }

    std::memcpy(r_buffer_,pkt->Data(),bytes);
    auto ret = srtp_unprotect(recv_ctx_,r_buffer_,&bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_unprotect failed.err:" << ret;
        return null_packet;
    }

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(),r_buffer_,bytes);
    npkt->SetPacketSize(bytes);
    return npkt;
}
PacketPtr Srtp::SrtcpUnprotect(const char *buf,size_t size)
{
    int32_t bytes = size;

    if(bytes >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large,bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }

    std::memcpy(r_buffer_,buf,size);
    auto ret = srtp_unprotect_rtcp(recv_ctx_,r_buffer_,&bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_unprotect_rtcp failed.err:" << ret;
        return null_packet;
    }

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(),r_buffer_,bytes);
    npkt->SetPacketSize(bytes);
    return npkt;
}
PacketPtr Srtp::SrtcpUnprotect( PacketPtr &pkt)
{
    int32_t bytes = pkt->PacketSize();

    if(bytes >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large,bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }

    std::memcpy(r_buffer_,pkt->Data(),bytes);
    auto ret = srtp_unprotect_rtcp(recv_ctx_,r_buffer_,&bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_unprotect_rtcp failed.err:" << ret;
        return null_packet;
    }

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(),r_buffer_,bytes);
    npkt->SetPacketSize(bytes);
    return npkt;
}
void Srtp::OnSrtpEvent(srtp_event_data_t* data)
{

}