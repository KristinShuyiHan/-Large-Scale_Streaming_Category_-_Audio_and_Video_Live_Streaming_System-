#include "Rtcp.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "mmedia/base/BytesWriter.h"

using namespace tmms::mm;

Rtcp::Rtcp(int32_t pt)
:version_(2),
padding_(0),
rc_(0),
payload_type_(pt),
pl_len_(0)
{

}
int32_t Rtcp::DecodeHeader(const char *data, size_t len)
{
    if(len<4)
    {
        return 0;
    }
    version_ = data[0]>>6;
    padding_ = (data[0]>>5)&0x01;
    rc_ = (data[0]&0x1F);
    payload_type_ = data[1];
    data += 2;
    pl_len_ = BytesReader::ReadUint16T(data);
    return 4;
}
int32_t Rtcp::EncodeHeader(char *buf)
{
    buf[0] = (char)((version_<<6)|(padding_<<5)|rc_);
    buf[1] = (char)payload_type_;

    BytesWriter::WriteUint16T(buf+2,pl_len_);
    return 4;
}    
void Rtcp::SetRC(int count)
{
    rc_ = count;
}
int  Rtcp::RC() const
{
    return rc_;
}
void Rtcp::SetPayloadType(int type)
{
    payload_type_ = type;
}
int Rtcp::PayloadType() const
{
    return payload_type_;
}
void Rtcp::SetPadding(bool pad)
{
    padding_ = pad?1:0;
}
bool Rtcp::Padding() const
{
    return padding_==0?false:true;
}
void Rtcp::SetPayloadLength(uint32_t size)
{
    pl_len_ = size/4;
}
uint32_t Rtcp::PayloadLength() const
{
    return pl_len_*4;
}