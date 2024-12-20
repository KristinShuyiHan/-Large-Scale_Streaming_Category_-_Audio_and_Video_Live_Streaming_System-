#pragma once

#include "Fragment.h"
#include "FragmentWindow.h"
#include "mmedia/mpegts/TsEncoder.h"
#include "mmedia/base/Packet.h"
#include <cstdint>
#include <string>
#include <memory>

namespace tmms
{
    namespace mm
    {
        class HLSMuxer
        {
        public:
            HLSMuxer(const string &session_name);
            ~HLSMuxer() = default;

            string PlayList();
            void OnPacket(PacketPtr &packet);
            FragmentPtr GetFragment(const string &name);
            void ParseCodec(FragmentPtr &fragment,PacketPtr &packet);

        private:
            static bool IsCodecHeader(const PacketPtr &packet);    
            FragmentWindow fragment_window_;
            TsEncoder encoder_;
            FragmentPtr current_fragment_;
            std::string stream_name_;
            int64_t fragment_seq_no_{0};
            int32_t min_fragment_size_{3000};
            int32_t max_fragment_size_{12000};
        };
    }
}