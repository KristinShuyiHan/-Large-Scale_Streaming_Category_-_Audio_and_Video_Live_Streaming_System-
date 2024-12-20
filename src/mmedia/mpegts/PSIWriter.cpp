#include "PSIWriter.h"
#include "mmedia/base/BytesWriter.h"
#include "TsTool.h"
#include <cstring>

using namespace tmms::mm;

void PSIWriter::SetVersion(uint8_t v)
{
    version_ = v;
}
int PSIWriter::WriteSection(StreamWriter * w, int id, int sec_num, int last_sec_num,uint8_t *buf, int len)
{
    uint8_t section[1024],*p;
    auto total_len = len+3+5+4;

    p = section;
    *p++ = table_id_;
    BytesWriter::WriteUint16T((char*)p,(5+4+len)|0xb000);
    p += 2;
    BytesWriter::WriteUint16T((char*)p,id);
    p += 2;

    *p++ = 0xc1|(version_<<1);
    *p++ = sec_num;
    *p++ = last_sec_num;
    memcpy(p,buf,len);
    p += len;
    auto crc = TsTool::CRC32(section,total_len-4);
    BytesWriter::WriteUint32T((char*)p,crc);
    PushSection(w,section,total_len);
    return 0;
}
void PSIWriter::PushSection(StreamWriter * w,uint8_t *buf, int len)
{
    uint8_t packet[188],*q;
    uint8_t * p = buf;
    bool first = false;

    while(len>0)
    {
        q = packet;
        first = (p == buf);
        *q ++ = 0x47;
        auto b = pid_>>8;
        if(first)
        {
            b |= 0x40;
        }
        *q ++ = b;
        *q ++ = pid_;
        cc_ = (cc_+1)&0xf;
        *q ++ = 0x10|cc_;
        if(first)
        {
            *q++ = 0;
        }

        auto len1 = 188 - (q-packet);
        if(len1 > len)
        {
            len1 = len;
        }
        memcpy(q,p,len1);
        q += len1;

        auto left = 188 - (q - packet);
        if(left>0)
        {
            memset(q,0xff,left);
        }

        w->Write(packet,188);
        p += len1;
        len -= len1;
    }
}