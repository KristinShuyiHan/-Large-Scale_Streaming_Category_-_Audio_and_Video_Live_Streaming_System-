#include "FragmentWindow.h"
#include "mmedia/base/MMediaLog.h"
#include <sstream>
#include <cmath>

using namespace tmms::mm;

namespace
{
    static FragmentPtr fragment_null;
}

FragmentWindow::FragmentWindow(int32_t size)
:window_size_(size)
{

}
FragmentWindow::~FragmentWindow()
{

}

void FragmentWindow::AppendFragment(FragmentPtr &&fragment)
{
    {
        std::lock_guard<std::mutex> lk(lock_);
        fragments_.emplace_back(std::move(fragment));
    }
    Shrink();
    UpdatePlayList();
}
FragmentPtr FragmentWindow::GetIdleFragment()
{
    std::lock_guard<std::mutex> lk(lock_);
    if(free_fragments_.empty())
    {
        return std::make_shared<Fragment>();
    }
    else
    {
        auto p = free_fragments_[0];
        free_fragments_.erase(free_fragments_.begin());
        return p;
    }
}
const FragmentPtr &FragmentWindow::GetFragmentByName(const string &name)
{
    std::lock_guard<std::mutex> lk(lock_);
    for(auto &f:fragments_)
    {
        if(f->FileName() == name)
        {
            return f;
        }
    }
    return fragment_null;
}
string FragmentWindow::GetPlayList()
{
    std::lock_guard<std::mutex> lk(lock_);
    return playlist_;
}
void FragmentWindow::Shrink()
{
    std::lock_guard<std::mutex> lk(lock_);
    int remove_index = -1;
    if(fragments_.size() <= window_size_)
    {
        return ;
    }
    remove_index = fragments_.size() - window_size_;

    for(int i = 0;i<remove_index && !fragments_.empty();i++)
    {
        auto p = *fragments_.begin();
        fragments_.erase(fragments_.begin());
        p->Reset();
        free_fragments_.emplace_back(std::move(p));
    }
}
void FragmentWindow::UpdatePlayList()
{
    std::lock_guard<std::mutex> lk(lock_);
    if(fragments_.empty()||fragments_.size() < 3)
    {
        return ;
    }

    std::ostringstream ss;

    ss << "#EXTM3U\n#EXT-X-VERSION:3 \n";

    int i = fragments_.size()>5?(fragments_.size() - 5):0;
    int j = i;
    int32_t max_duration = 0;
    for(;j<fragments_.size();j++)
    {
        max_duration = std::max(max_duration,(int32_t)fragments_[j]->Duration());
    }

    int32_t target_duration = (int32_t)ceil((max_duration/1000.0));

    ss << "#EXT-X-TARGETDURATION:" << target_duration << "\n";
    ss << "#EXT-X-MEDIA-SEQUENCE:" << fragments_[i]->SequenceNo() << "\n";
    ss.precision(3);
    ss.setf(std::ios::fixed,std::ios::floatfield);
    for(;i<fragments_.size();i++)
    {
        ss << "#EXTINF:" << fragments_[i]->Duration()/1000.0 << "\n";
        ss << fragments_[i]->FileName() << "\n";
    }

    playlist_.clear();
    playlist_ = std::move(ss.str());
    HLS_TRACE << "playlist:\n" << playlist_;
}