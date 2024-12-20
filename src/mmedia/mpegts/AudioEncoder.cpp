#include "AudioEncoder.h"
#include "mmedia/base/MMediaLog.h"
#include "TsTool.h"

using namespace tmms::mm;

namespace
{
    AacProfile AacObjectType2AacProfile(AACObjectType type)
    {
        switch (type)
        {
        case kAACObjectTypeAacMain:
            return AacProfileMain;
        case kAACObjectTypeAacHE:
        case kAACObjectTypeAacHEV2:
        case kAACObjectTypeAacLC:
            return AacProfileLC;
        case kAACObjectTypeAacSSR:
            return AacProfileSSR;        
        default:
            break;
        }
        return AacProfileReserved;
    }
}

int32_t AudioEncoder::EncodeAudio(StreamWriter* writer, PacketPtr &data,int64_t dts)
{
    std::list<SampleBuf> list;
    auto ret = demux_.OnDemux(data->Data(),data->PacketSize(),list);
    if(ret == -1)
    {
        MPEGTS_ERROR << "audio demux err.";
        return -1;
    }

    if(TsTool::IsCodecHeader(data))
    {
        return 0;
    }

    writer->AppendTimeStamp(dts);
    dts = dts * 90;
    if(demux_.GetCodecId() == kAudioCodecIDAAC)
    {
        return EncodeAAC(writer,list,dts);
    }
    else if(demux_.GetCodecId() == kAudioCodecIDMP3)
    {
        return EncodeMP3(writer,list,dts);
    }
    return 0;
}
void AudioEncoder::SetPid(uint16_t pid)
{
    pid_ = pid;
}
void AudioEncoder::SetStreamType (TsStreamType type)
{
    type_ = type;
}
int32_t AudioEncoder::EncodeAAC(StreamWriter* writer,std::list<SampleBuf> &sample_list,int64_t pts)
{
    for(auto const &l:sample_list)
    {
        if(l.size<=0||l.size > 0x1fff)
        {
            MPEGTS_ERROR << "invalid aac frame length.";
            return -1;
        }
        int32_t frame_length = 7+ l.size;

        std::list<SampleBuf> result;
        uint8_t adts_header[7] = {0xff,0xf9,0x00,0x00,0x00,0x0f,0xfc};

        AacProfile profile = AacObjectType2AacProfile(demux_.GetObjectType());

        adts_header[2] = (profile << 6)&0xc0;
        adts_header[2] |= (demux_.GetSampleRateIndex()<<2)&0x3c;
        adts_header[2] |= (demux_.GetChannel()>>2)&0x01;
        adts_header[3] = (demux_.GetChannel()<<6)&0xc0;

        adts_header[3] |= (frame_length>>11)&0x03;
        adts_header[4] = (frame_length >> 3) &0xff;
        adts_header[5] = (frame_length<<5)&0xe0;

        adts_header[5] |= 0x1f;

        result.emplace_back(SampleBuf((const char*)adts_header,7));
        result.emplace_back(l.addr,l.size);

        return WriteAudioPes(writer,result,7+l.size,pts);
    }
    return 0;
}
int32_t AudioEncoder::EncodeMP3(StreamWriter* writer,std::list<SampleBuf> &sample_list,int64_t pts)
{
    int32_t size = 0;
    for(auto const &l:sample_list)
    {
        size += l.size;
    }
    return WriteAudioPes(writer,sample_list,size,pts);
}
int32_t AudioEncoder::WriteAudioPes(StreamWriter* writer,std::list<SampleBuf> &result, int payload_size,int64_t dts)
{
    uint8_t buf[188],*q;

    int32_t val = 0;
    bool is_start = true;

    while(payload_size>0&&!result.empty())
    {
        memset(buf,0x00,188);

        q = buf;
        *q++ = 0x47;

        val = pid_>>8;
        if(is_start)
        {
            val |= 0x40;
        }
        *q ++ = val;
        *q ++ = pid_;
        cc_ = (cc_+1)&0xf;
        *q ++ = 0x10|cc_;

        if(is_start)
        {
            *q ++ = 0x00;
            *q ++ = 0x00;
            *q ++ = 0x01;

            *q ++= 0xc0;

            int32_t len = payload_size + 5 + 3;
            if(len>0xffff)
            {
                len = 0;
            }

            *q ++ = len >> 8;
            *q ++ = len;
            *q ++ = 0x80;
            *q ++ = 0x02<<6;
            *q ++ = 5;

            TsTool::WritePts(q,0x02,dts);
            q += 5;

            is_start = false;
        }

        int32_t header_len = q - buf;
        int32_t len = 188 - header_len;
        if(len > payload_size)
        {
            len = payload_size;
        }
        int32_t stuffing = 188 - header_len - len;
        if(stuffing>0)
        {
            if(buf[3]&0x20)
            {
                int32_t af_len = buf[4] + 1;
                memmove(buf+4+af_len+stuffing,buf+4+af_len,header_len -(4+af_len));
                buf[4] += stuffing;
                memset(buf+4+af_len,0xff,stuffing);
            }
            else 
            {
                memmove(buf+4+stuffing,buf+4,header_len -4);
                buf[3] |= 0x20;
                buf[4] = stuffing - 1;
                if(stuffing > 2)
                {
                    buf[5] = 0x00;
                    memset(buf+6,0xff,stuffing - 2);
                }
            }
        }
        auto slen = len;
        while(slen>0&&!result.empty())
        {
            auto &sbuf = result.front();
            if(sbuf.size <= slen)
            {
                memcpy(buf+188-slen,sbuf.addr,sbuf.size);
                slen -= sbuf.size;
                result.pop_front();
            }
            else 
            {
                memcpy(buf+188-slen,sbuf.addr,slen);
                sbuf.addr += slen;
                sbuf.size -= slen;
                slen = 0;
                break;
            }
        }
        payload_size -= len;
        writer->Write(buf,188);
    }
    return 0;
}
