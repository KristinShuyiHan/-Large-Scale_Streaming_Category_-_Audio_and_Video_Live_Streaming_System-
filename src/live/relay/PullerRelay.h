#pragma once

#include "live/relay/pull/Puller.h"
#include "live/Session.h"
#include <vector>

namespace tmms
{
    namespace live
    {
        class PullerRelay:public PullHandler
        {
        public:
            PullerRelay(Session &s);
            ~PullerRelay();
            void StartPullStream();
        private:
            void OnPullSucess() override;
            void OnPullClose() override;
            bool GetTargets();
            Puller *GetPuller(TargetPtr p);
            void SelectTarget();
            void Pull();
            void ClearPuller();

            Session &session_;
            std::vector<TargetPtr> targets_;
            TargetPtr current_target_;
            int32_t cur_target_index_{-1};
            Puller * puller_{nullptr};
            EventLoop *current_loop_{nullptr};
        };
    }
}