#include "FlvPlayerUser.h"
#include "live/Stream.h"
#include "mmedia/flv/FlvContext.h"
#include "live/base/LiveLog.h"
#include "base/TTime.h"

using namespace tmms::live;
using namespace tmms::mm;
using FlvContextPtr = std::shared_ptr<FlvContext>;

FlvPlayerUser::FlvPlayerUser(const ConnectionPtr &ptr,const StreamPtr &stream,const SessionPtr &s)
:PlayerUser(ptr,stream,s)
{

}
void FlvPlayerUser::PushFlvHttpHeader()
{
    auto cxt = connection_->GetContext<FlvContext>(kFlvContext);
    if(cxt)
    {
        bool has_video = stream_->HasVideo();
        bool has_audio = stream_->HasAudio();
        cxt->SendFlvHttpHeader(has_video,has_audio);
        http_header_sent_ = true;
    }
}
bool FlvPlayerUser::PostFrames()
{
    if(!stream_->Ready()||!stream_->HasMedia())
    {
        Deactive();
        return false;
    }
    if(!http_header_sent_)
    {
        PushFlvHttpHeader();
        return false;
    }
    stream_->GetFrames(std::dynamic_pointer_cast<PlayerUser>(shared_from_this()));
    if(meta_)
    {
        auto ret = PushFrame(meta_,true);
        if(ret)
        {
            LIVE_INFO << "rtmp sent meta now:" << base::TTime::NowMS() << " host:" << user_id_;
            meta_.reset();
        }
    }
    else if(audio_header_)
    {
        auto ret = PushFrame(audio_header_,true);
        if(ret)
        {
            LIVE_INFO << "rtmp sent audio_header now:" << base::TTime::NowMS() << " host:" << user_id_;
            audio_header_.reset();
        }
    }    
    else if(video_header_)
    {
        auto ret = PushFrame(video_header_,true);
        if(ret)
        {
            LIVE_INFO << "rtmp sent video_header now:" << base::TTime::NowMS() << " host:" << user_id_;
            video_header_.reset();
        }
    } 
    else if(!out_frames_.empty())  
    {
        auto ret = PushFrames(out_frames_);
        if(ret)
        {
            out_frames_.clear();
        }
    }  
    else 
    {
        Deactive();
    }
    return true;
}
UserType FlvPlayerUser::GetUserType() const
{
    return UserType::kUserTypePlayerFlv;
}

bool FlvPlayerUser::PushFrame(PacketPtr &packet,bool is_header)
{
    auto cx = connection_->GetContext<FlvContext>(kFlvContext);
    if(!cx||!cx->Ready())
    {
        return false;
    }
    int64_t ts = 0;
    if(!is_header)
    {
        ts = time_corrector_.CorrectTimestamp(packet);
    }

    cx->BuildFlvFrame(packet,ts);
    cx->Send();
    return true;
}

bool FlvPlayerUser::PushFrames(std::vector<PacketPtr> &list)
{
    auto cx = connection_->GetContext<FlvContext>(kFlvContext);
    if(!cx||!cx->Ready())
    {
        return false;
    }
    int64_t ts = 0;
    for(int i = 0;i<list.size();i++)
    {
        PacketPtr &packet = list[i];
        ts = time_corrector_.CorrectTimestamp(packet);
        cx->BuildFlvFrame(packet,ts);
    }
    
    cx->Send();
    return true;
}
