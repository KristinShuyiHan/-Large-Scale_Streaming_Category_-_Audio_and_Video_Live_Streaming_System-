#include "RtpOpus.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

RtpOpus::RtpOpus(int32_t pt)
:Rtp(pt)
{
    sample_ = 48000;
}

bool RtpOpus::Encode(std::list<SampleBuf> &ins,uint32_t ts,std::list<PacketPtr> &outs)
{
    int32_t header_size = HeaderSize();
    for(auto const&s:ins)
    {
        int32_t payload_size = header_size + s.size;

        PacketPtr packet = Packet::NewPacket(payload_size);
        char *header = packet->Data();
        char *payload = header + header_size;

        timestamp_ += 10*(sample_/1000.0);
        sequence_ = sequence_ + 1;
        marker_ = 1;
        EncodeHeader(header);

        memcpy(payload,s.addr,s.size);
        packet->SetPacketSize(payload_size);
        packet->SetPacketType(kPacketTypeAudio);
        packet->SetIndex(sequence_);
        outs.emplace_back(std::move(packet));
    }
    return true;
}
