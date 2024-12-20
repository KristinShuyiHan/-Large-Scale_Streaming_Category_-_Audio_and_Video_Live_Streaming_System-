#include "Rtpfb.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

Rtpfb::Rtpfb()
:Rtcp(kRtcpPtRtpfb)
{

}
uint32_t Rtpfb::Ssrc() const
{
    return ssrc_;
}
uint32_t Rtpfb::MediaSsrc() const
{
    return media_ssrc_;
}
const std::set<uint16_t,SeqCmp> &Rtpfb::LostSeqs()const
{
    return lost_seqs_;
}
size_t Rtpfb::DecodeNack(const char *data, size_t len)
{
    const char *p = data;
    lost_seqs_.clear();
    while(len >= 4)
    {
        auto pid = BytesReader::ReadUint16T(p);
        WEBRTC_DEBUG << "pid:" << pid;
        auto blp = BytesReader::ReadUint16T(p+2);
        
        lost_seqs_.insert(pid);
        for(int i = 0;i<16;i++)
        {
            if((blp>>i)&0x0001)
            {
                uint16_t npid = pid+i+1;
                WEBRTC_DEBUG << "blp:" << blp << " npid:" << npid;
                lost_seqs_.insert(npid);
            }
        }
        p += 4;
        len -= 4;
    }
    return p-data;
}
void Rtpfb::Decode(const char *data, size_t len) 
{
    auto parsed = DecodeHeader(data,len);
    if(parsed == 0)
    {
        return ;
    }

    data += parsed;
    len -= parsed;

    if(len < 8)
    {
        WEBRTC_ERROR << "rtpfb no enough len:"<< len;
        return;
    }

    ssrc_ = BytesReader::ReadUint32T(data);
    media_ssrc_ = BytesReader::ReadUint32T(data + 4);

    data += 8;
    len -= 8;

    if(rc_ == 1)
    {
        DecodeNack(data,len);
    }
}
PacketPtr Rtpfb::Encode() 
{
    return PacketPtr();
}