#pragma once

#include "PlayerUser.h"

namespace tmms
{
    namespace live
    {
        class FlvPlayerUser:public PlayerUser
        {
        public:
            explicit FlvPlayerUser(const ConnectionPtr &ptr,const StreamPtr &stream,const SessionPtr &s);

            bool PostFrames();
            UserType GetUserType() const;
        private:
            using User::SetUserType;

            bool PushFrame(PacketPtr &packet,bool is_header);
            bool PushFrames(std::vector<PacketPtr> &list);
            void PushFlvHttpHeader();
            bool http_header_sent_{false};
        };
    }
}