#include "RtpMuxer.h"
#include "RtpOpus.h"
#include "RtpH264.h"
#include "mmedia/mpegts/TsTool.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

bool RtpMuxer::Init(int32_t vpt,int32_t apt,uint32_t vssrc,uint32_t assrc)
{
    audio_rtp_ = std::make_shared<RtpOpus>(apt);
    video_rtp_ = std::make_shared<RtpH264>(vpt);
    audio_demux_ = std::make_shared<AudioDemux>();
    video_demux_ = std::make_shared<VideoDemux>();
    audio_rtp_->SetSsrc(assrc);
    video_rtp_->SetSsrc(vssrc);
    return true;
}
int32_t RtpMuxer::EncodeVideo(PacketPtr &pkt, std::list<PacketPtr>&rtp_pkts, uint32_t timestamp)
{
    std::list<SampleBuf> list;
    video_demux_->Reset();
    auto ret = video_demux_->OnDemux(pkt->Data(),pkt->PacketSize(),list);
    if(ret == -1)
    {
        WEBRTC_ERROR << "video demux error.";
        return -1;
    }
    if(TsTool::IsCodecHeader(pkt))
    {
        return 0;
    }
    if(video_demux_->HasBFrame())
    {
        return 0;
    }
    if(!sent_sps_pps_&&video_demux_->HasIdr()&&!video_demux_->HasSpsPps())
    {
        sent_sps_pps_ = true;
        auto h264_rtp = std::dynamic_pointer_cast<RtpH264>(video_rtp_);
        h264_rtp->EncodeSpsPps(video_demux_->GetSPS(),video_demux_->GetPPS(),rtp_pkts);
    }
    if(video_demux_->HasSpsPps())
    {
        sent_sps_pps_ = true;
    }
    auto r = video_rtp_->Encode(list,timestamp,rtp_pkts);
    if(!r)
    {
        return -1;
    }
    return 0;
}
int32_t RtpMuxer::EncodeAudio(PacketPtr &pkt, std::list<PacketPtr> &rtp_pkts, uint32_t timestamp)
{
    std::list<SampleBuf> list;
    auto ret = audio_demux_->OnDemux(pkt->Data(),pkt->PacketSize(),list);
    if(ret == -1)
    {
        WEBRTC_ERROR << "audio demux error.";
        return -1;
    }
    if(audio_demux_->AACSeqHeaer().empty())
    {
        return 0;
    }
    if(!aac_decoder_)
    {
        aac_decoder_ = std::make_shared<AACDecoder>();
        aac_decoder_->Init(audio_demux_->AACSeqHeaer());
    }
    if(!opus_encoder_)
    {
        opus_encoder_ = std::make_shared<TOpusEncoder>();
        opus_encoder_->Init(audio_demux_->GetSampleRate(),audio_demux_->GetChannel());
    }
    for(auto &l:list)
    {
        auto pcm = aac_decoder_->Decode((unsigned char*)l.addr,l.size);
        if(pcm.size>0)
        {
            std::list<PacketPtr> outs;
            auto r = opus_encoder_->Encode(pcm,outs);
            if(r)
            {
                std::list<SampleBuf> result;
                for(auto &o:outs)
                {
                    result.emplace_back(o->Data(),o->PacketSize());
                }
                audio_rtp_->Encode(result,timestamp,rtp_pkts);
            }
        }
    }
    return 0;
}

uint32_t RtpMuxer::VideoTimestamp() const
{
    return video_rtp_->Timestamp();
}
uint32_t RtpMuxer::AudioTimestamp() const
{
    return audio_rtp_->Timestamp();
}
uint32_t RtpMuxer::VideoSsrc() const
{
    return video_rtp_->Ssrc();
}
uint32_t RtpMuxer::AudioSsrc() const
{
    return audio_rtp_->Ssrc();
}
