#include "VideoEncoder.h"
#include "mmedia/base/MMediaLog.h"
#include "TsTool.h"

using namespace tmms::mm;

namespace 
{
    uint8_t *get_start_payload(uint8_t *pkt)
    {
        if(pkt[3]&0x20)
        {
            return pkt + 5 + pkt[4];
        }
        else
        {
            return pkt+4;
        }
    }
    int WritePcr(uint8_t *buf, int64_t pcr)
    {
        int64_t pcrv = (0) & 0x1ff;
        pcrv |= (0x3f << 9) & 0x7E00;
        pcrv |= ((pcr) << 15) & 0xFFFFFFFF8000LL;

        char *pp = (char*)&pcrv;
        *buf++ = pp[5];
        *buf++ = pp[4];
        *buf++ = pp[3];
        *buf++ = pp[2];
        *buf++ = pp[1];
        *buf++ = pp[0];

        return 6;
    }

}
int32_t VideoEncoder::EncodeVideo(StreamWriter* writer, bool key,PacketPtr &data,int64_t dts)
{
    std::list<SampleBuf> list;
    auto ret = demux_.OnDemux(data->Data(),data->PacketSize(),list);
    if(ret == -1)
    {
        MPEGTS_ERROR << "video demux error.";
        return -1;
    }
    if(TsTool::IsCodecHeader(data))
    {
        return 0;
    }
    writer->AppendTimeStamp(dts);
    dts = dts* 90;
    if(demux_.GetCodecID() == kVideoCodecIDAVC)
    {
        return EncodeAvc(writer,list,key,dts);
    }
    return 0;
}
void VideoEncoder::SetPid(uint16_t pid)
{
    pid_ = pid;
}
void VideoEncoder::SetStreamType (TsStreamType type)
{
    type_ = type;
}
int32_t VideoEncoder::EncodeAvc(StreamWriter* writer,std::list<SampleBuf> &sample_list,bool key,int64_t dts)
{
    int32_t total_size = 0;
    std::list<SampleBuf> result;
    bool startcode_inserted = true;
    if(!demux_.HasAud())
    {
        static uint8_t default_aud_nalu[] = {0x09,0xf0};
        static SampleBuf default_aud_buf((const char*)&default_aud_nalu[0],2);
        total_size += AvcInsertStartCode(result,startcode_inserted);
        result.push_back(default_aud_buf);
        total_size += 2;
    }
    for(auto const&l:sample_list)
    {
        if(l.size<=0)
        {
            MPEGTS_ERROR << "invalid avc frame length.";
            continue;
        }

        auto bytes = l.addr;
        NaluType type = (NaluType)(bytes[0]&0x1f);
        if(type == kNaluTypeIDR&&!demux_.HasSpsPps()&&
            (writer->GetSPS() != demux_.GetSPS()||
             writer->GetPPS() != demux_.GetPPS()||
             !writer->GetSpsPpsAppended()))
        {
            auto const &sps = demux_.GetSPS();
            if(!sps.empty())
            {
                total_size += AvcInsertStartCode(result,startcode_inserted);
                result.emplace_back(sps.data(),sps.size());
                total_size += sps.size();
                writer->SetSPS(sps);
            }
            else
            {
                MPEGTS_ERROR << "no sps";
            }
            auto const &pps = demux_.GetPPS();
            if(!pps.empty())
            {
                total_size += AvcInsertStartCode(result,startcode_inserted);
                result.emplace_back(pps.data(),pps.size());
                total_size += pps.size();
                writer->SetPPS(pps);
            }
            else
            {
                MPEGTS_ERROR << "no pps";
            }   
            writer->SetSpsPpsAppended(true);       
        }
        total_size += AvcInsertStartCode(result,startcode_inserted);
        result.emplace_back(l.addr,l.size);
        total_size += l.size;        
    }
    int64_t pts = dts;
    if(demux_.GetCST()>0)
    {
        pts = dts + demux_.GetCST()*90;
    }
    return WriteVideoPes(writer,result,total_size,pts,dts,key);
}
int32_t VideoEncoder::AvcInsertStartCode(std::list<SampleBuf> &sample_list,bool &startcode_inserted)
{
    if(startcode_inserted)
    {
        static uint8_t default_start_nalu[] = {0x00,0x00,0x01};
        static SampleBuf default_start_buf((const char*)&default_start_nalu[0],3);
        sample_list.emplace_back(default_start_buf);
        return 3;
    }
    else
    {
        static uint8_t default_start_nalu[] = {0x00,0x00,0x00,0x01};
        static SampleBuf default_start_buf((const char*)&default_start_nalu[0],4);
        sample_list.emplace_back(default_start_buf);
        startcode_inserted = true;
        return 4;        
    }
}
int32_t VideoEncoder::WriteVideoPes(StreamWriter* writer,std::list<SampleBuf> &result, int payload_size,int64_t pts, int64_t dts, bool key)
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
            if(key)
            {
                buf[3] |= 0x20;
                buf[4] = 1;
                buf[5] = 0x10;

                q = get_start_payload(buf);
                auto size = WritePcr(q,pts);
                buf[4] += size;
                q = get_start_payload(buf);
            }
            *q ++ = 0x00;
            *q ++ = 0x00;
            *q ++ = 0x01;

            *q ++= 0xe0;

            int16_t header_len = 5;
            uint8_t flags = 0x02;

            if(pts != dts)
            {
                header_len += 5;
                flags = 0x03;
            }

            int32_t len = payload_size + header_len + 3;
            if(len>0xffff)
            {
                len = 0;
            }

            *q ++ = len >> 8;
            *q ++ = len;
            *q ++ = 0x80;
            *q ++ = flags<<6;
            *q ++ = header_len;

            if(flags == 0x02)
            {
                TsTool::WritePts(q,0x02,pts);
                q += 5;
            }
            else if(flags == 0x03)
            {
                TsTool::WritePts(q,0x03,pts);
                q += 5;
                TsTool::WritePts(q,0x01,dts);
                q += 5;                
            }

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
