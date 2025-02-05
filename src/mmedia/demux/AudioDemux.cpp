#include "AudioDemux.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/NalBitStream.h"
using namespace tmms::mm;

int32_t AudioDemux::OnDemux(const char *data,size_t size,std::list<SampleBuf> &list)
{
    if(size<1)
    {
        DEMUX_ERROR << "param error.size<1";
        return -1;
    }
    sound_format_ = (AudioCodecID) ((*data>>4)&0x0f);
    sound_rate_ = (SoundRate)((*data&0xc0)>>2);
    sound_size_ = (SoundSize)((*data&0x02)>>1);
    sound_type_ = (SoundChannel)(*data&0x01);
    // DEMUX_DEBUG<< "format:" << sound_format_
    //             << " rate:" << sound_rate_
    //             << " size:" << sound_size_
    //             << " type:" << sound_type_;
    if(sound_format_ == kAudioCodecIDMP3)
    {
        return DemuxMP3(data,size,list);
    }
    else if(sound_format_ == kAudioCodecIDAAC)
    {
        return DemuxAAC(data,size,list);
    }
    else 
    {
        DEMUX_ERROR << "not surpport code id:" << sound_format_;
    }
    return -1;
}         
int32_t AudioDemux::DemuxAAC(const char *data,size_t size,std::list<SampleBuf> &list)
{
    AACPacketType type = (AACPacketType)data[1];

    if(type == kAACPacketTypeAACSequenceHeader)
    {
        if(size - 2 > 0)
        {
            aac_seq_header_.clear();
            aac_seq_header_.assign(data+2,size - 2);
            return DemuxAACSequenceHeader(data+2,size - 2);
        }
    }
    else if(type == kAACPacketTypeAACRaw)
    {
        if(!aac_ok_)
        {
            return -1;
        }
        list.emplace_back(SampleBuf(data+2,size - 2));
    }
    return 0;
}
int32_t AudioDemux::DemuxMP3(const char *data,size_t size,std::list<SampleBuf> &list)
{
    list.emplace_back(SampleBuf(data+1,size-1));
    return 0;
}
int32_t AudioDemux::DemuxAACSequenceHeader(const char* data, int size)
{
    if(size < 2)
    {
        DEMUX_ERROR << "demux aac seq header failed. size < 2";
        return -1;
    }
    NalBitStream stream(data,size);

    aac_object_ = (AACObjectType)stream.GetWord(5);
    aac_sample_rate_ = stream.GetWord(4);
    aac_channel_ = stream.GetWord(4);

    aac_ok_ = true;
    return 0;
}
int32_t AudioDemux::GetSampleRate() const
{
    const int aac_sample_rates[16] = 
    {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000, 7350
    };
    return aac_sample_rates[aac_sample_rate_];
}

const std::string &AudioDemux::AACSeqHeaer() const
{
    return aac_seq_header_;
}