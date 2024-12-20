#include "SenderReport.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesWriter.h"

using namespace tmms::mm;

SenderReport::SenderReport()
:Rtcp(kRtcpPtSR)
{

}
PacketPtr SenderReport::Encode()
{
    int32_t packet_size = 4 + 24;

    PacketPtr packet = Packet::NewPacket(packet_size);
    packet->SetPacketSize(packet_size);
    char *header = packet->Data();
    char *payload = header + 4;

    SetPayloadLength(24);
    EncodeHeader(header);

    BytesWriter::WriteUint32T(payload,ssrc_);
    BytesWriter::WriteUint32T(payload+4,ntp_mword_);
    BytesWriter::WriteUint32T(payload+8,ntp_lword_);
    BytesWriter::WriteUint32T(payload+12,rtp_timestamp_);
    BytesWriter::WriteUint32T(payload+16,sent_pkt_cnt_);
    BytesWriter::WriteUint32T(payload+20,sent_bytes_);

    return packet;
}
void SenderReport::SetSsrc(uint32_t ssrc)
{
    ssrc_ = ssrc;
}
void SenderReport::SetNtpTimestamp(uint64_t time)
{
    ntp_mword_ = time>>32;
    ntp_lword_ = time&0xffffffff;
}
void SenderReport::SetRtpTimestamp(uint32_t time)
{
    rtp_timestamp_ = time;
}
void SenderReport::SetSentPacketCount(uint32_t count)
{
    sent_pkt_cnt_ = count;
}
void SenderReport::SetSentBytes(uint32_t bytes)
{
    sent_bytes_ = bytes;
}