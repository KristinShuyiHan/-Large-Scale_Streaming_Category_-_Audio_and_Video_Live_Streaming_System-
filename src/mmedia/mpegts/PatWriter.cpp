#include "PatWriter.h"
#include "mmedia/base/BytesWriter.h"

using namespace tmms::mm;

PatWriter::PatWriter()
{
    pid_ = 0x0000;
    table_id_ = 0x00;
}
void PatWriter::WritePat(StreamWriter * w)
{
    uint8_t section[kSectionMaxSize],*q;
    q = section;
    BytesWriter::WriteUint16T((char*)q,program_number_);
    q += 2;
    BytesWriter::WriteUint16T((char*)q,0xe000|pmt_pid_);
    q += 2;

    PSIWriter::WriteSection(w,transport_stream_id_,0,0,section,q - section);
}