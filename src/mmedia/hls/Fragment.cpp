#include "Fragment.h"
#include "mmedia/base/MMediaLog.h"
#include "base/TTime.h"
#include <unistd.h>
#include <fcntl.h>

using namespace tmms::mm;

void Fragment::AppendTimeStamp(int64_t dts)
{
    if(start_dts_ == -1)
    {
        start_dts_ = dts;
    }
    start_dts_ = std::min(start_dts_,dts);
    duration_ = dts - start_dts_;
}
void Fragment::Save()
{
    if(data_&&data_size_>0)
    {
        int fd = ::open(filename_.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if(fd == -1) 
        {
            HLS_ERROR << "open slice failed, filename:" << filename_ << ", err:" << strerror(errno);
            return ;
        }
        
        int ret = ::write(fd, data_->Data(), data_->PacketSize());
        if(ret != data_size_) 
        {
            HLS_ERROR << "write ts failed, filename:" << filename_ << ", err:" << strerror(errno);
        }
        ::close(fd);
    }
}
int32_t Fragment::Write(void* buf, uint32_t size)
{
    if(!data_)
    {
        data_ = Packet::NewPacket(buf_size_);
    }
    if(data_->Space() < size)
    {
        buf_size_ += kFragmentStepSize;
        while(data_size_+size>buf_size_)
        {
            buf_size_ += kFragmentStepSize;
        }
        PacketPtr new_pkt = Packet::NewPacket(buf_size_);
        memcpy(new_pkt->Data(),data_->Data(),data_->PacketSize());
        new_pkt->SetPacketSize(data_->PacketSize());
        data_ = new_pkt;
    }

    memcpy(data_->Data()+data_->PacketSize(),buf,size);
    data_->UpdatePacketSize(size);
    data_size_ += size;
    return size;
}
int32_t Fragment::Size()
{
    return data_->PacketSize();
}
char* Fragment::Data()
{
    return data_->Data() + data_->PacketSize();
}

int64_t Fragment::Duration() const
{
    return duration_;
}
const std::string &Fragment::FileName() const
{
    return filename_;
}
void Fragment::SetBaseFileName(const std::string &v)
{
    filename_.clear();
    filename_.append(v);
    filename_.append("_");
    filename_.append(std::to_string(base::TTime::NowMS()));
    filename_.append(".ts");
}
int32_t Fragment::SequenceNo() const
{
    return sequence_no_;
}
void Fragment::SetSequenceNo(int32_t no)
{
    sequence_no_ = no;
}
void Fragment::Reset()
{
    duration_ = 0;
    sequence_no_ = 0;
    data_size_ = 0;
    start_dts_ = -1;
    sps_pps_appended_ = false;
    if(data_)
    {
        data_->SetPacketSize(0);
    }
}
PacketPtr &Fragment::FragmentData()
{
    return data_;
}