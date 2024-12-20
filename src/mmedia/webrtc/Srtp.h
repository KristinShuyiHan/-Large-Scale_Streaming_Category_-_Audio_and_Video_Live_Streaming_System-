#pragma once

#include "mmedia/base/Packet.h"
#include "srtp2/srtp.h"
#include <cstdint>
#include <string>

namespace tmms
{
    namespace mm
    {
        const int32_t kSrtpMaxBufferSize = 65535;
        class Srtp
        {
        public:
            Srtp()=default;
            ~Srtp()=default;

            static bool InitSrtpLibrary();
            bool Init(const std::string &recv_key,const std::string &send_key);
            PacketPtr RtpProtect(PacketPtr &pkt);
            PacketPtr RtcpProtect( PacketPtr &pkt);
            PacketPtr SrtpUnprotect(PacketPtr &pkt);
            PacketPtr SrtcpUnprotect( PacketPtr &pkt); 
            PacketPtr SrtcpUnprotect(const char *buf,size_t size); 
        private:
            static void OnSrtpEvent(srtp_event_data_t* data);
            srtp_t send_ctx_{nullptr};
            srtp_t recv_ctx_{nullptr};
            char w_buffer_[kSrtpMaxBufferSize];
            char r_buffer_[kSrtpMaxBufferSize];
        };
    }
}