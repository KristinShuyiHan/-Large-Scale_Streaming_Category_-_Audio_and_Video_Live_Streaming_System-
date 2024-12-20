#include "RtpH264.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesWriter.h"

using namespace tmms::mm;

RtpH264::RtpH264(int32_t pt)
:Rtp(pt)
{
    sample_ = 90000;
}
bool RtpH264::Encode(std::list<SampleBuf> &ins,uint32_t ts,std::list<PacketPtr> &outs)
{
    timestamp_ = ts * (sample_/1000.0);
    marker_ = 0;
    int32_t header_size = HeaderSize();
    for(auto iter=ins.begin();iter!=ins.end();)
    {
        auto pre = iter;
        iter ++;

        if((header_size+pre->size)>kRtpMaxPayloadSize)
        {
            EncodeFua(*pre,outs);
        }
        else
        {
            bool maker = false;
            if(iter == ins.end())
            {
                maker = true;
            }            
            EncodeSingle(*pre,maker,outs);
        }
    }
    return true;
}
bool RtpH264::EncodeSpsPps(const std::string &sps,const std::string &pps,std::list<PacketPtr> &outs)
{
    if(sps.empty()&&pps.empty())
    {
        return false;
    }

    int32_t header_size = HeaderSize();
    int32_t payload_size = header_size + 5 + sps.size() + pps.size();

    PacketPtr packet = Packet::NewPacket(payload_size);
    char * header = packet->Data();
    char *payload = header+header_size;

    sequence_ ++;
    marker_ = 0;
    EncodeHeader(header);

    uint8_t type = sps[0];
    payload[0] = (type&(~0x1f))|24;
    payload += 1;

    BytesWriter::WriteUint16T(payload,sps.size());
    payload += 2;
    memcpy(payload,sps.c_str(),sps.size());
    payload += sps.size();

    BytesWriter::WriteUint16T(payload,pps.size());
    payload += 2;
    memcpy(payload,pps.c_str(),pps.size());
    payload += pps.size();

    packet->SetPacketSize(payload_size);
    packet->SetIndex(sequence_);
    packet->SetPacketType(kPacketTypeVideo);

    outs.emplace_back(packet);
    return true;
}
bool RtpH264::EncodeSingle(const SampleBuf & buf,bool last, std::list<PacketPtr> &outs)
{
    int32_t head_size = HeaderSize();
    int32_t payload_size = head_size + buf.size;

    PacketPtr packet = Packet::NewPacket(payload_size);
    char * header = packet->Data();
    char *payload = header+head_size;

    sequence_ ++;
    marker_ = last?1:0;
    EncodeHeader(header);

    memcpy(payload,buf.addr,buf.size);
    packet->SetPacketSize(payload_size);
    packet->SetPacketType(kPacketTypeVideo);
    packet->SetIndex(sequence_);
    outs.emplace_back(std::move(packet));
    return true;
}
bool RtpH264::EncodeFua(const SampleBuf & buf,std::list<PacketPtr> &outs)
{
    int32_t header_size = HeaderSize();
    unsigned char type = *buf.addr;
    const char *data = buf.addr + 1;
    int32_t bytes = buf.size - 1;
    bool start = true;
    while (bytes>0)
    {
        int32_t packet_size = header_size + 2 + bytes;
        if(packet_size>kRtpMaxPayloadSize)
        {
            packet_size = kRtpMaxPayloadSize;
        }
        PacketPtr packet = Packet::NewPacket(packet_size);
        char * header = packet->Data();
        char *fheader = header + header_size;
        char *payload = fheader + 2;
        int32_t payload_size = packet_size - 2 - header_size;

        sequence_ ++;
        if(bytes<=payload_size)
        {
            marker_ = 1;
        }
        else 
        {
            marker_ = 0;
        }
        EncodeHeader(header);

        fheader[0] = (type&0x60)|28;
        fheader[1] = (type&0x1f);
        if(start)
        {
            start = false;
            fheader[1] |= 0x80;
        }
        else if(bytes <= payload_size) 
        {
            fheader[1] |= 0x40;
        }

        memcpy(payload,data,payload_size);
        packet->SetPacketSize(packet_size);
        packet->SetPacketType(kPacketTypeVideo);
        packet->SetIndex(sequence_);
        outs.emplace_back(std::move(packet));

        data += payload_size;
        bytes -= payload_size;
    }
    return true;
}