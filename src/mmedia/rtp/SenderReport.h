#pragma once

#include "Rtcp.h"

namespace tmms
{
    namespace mm
    {
        class SenderReport:public Rtcp
        {
        public:
            SenderReport();
            ~SenderReport()=default;

            void Decode(const char *data, size_t len) override
            {

            }

            PacketPtr Encode() override; 
            void SetSsrc(uint32_t ssrc);
            void SetNtpTimestamp(uint64_t time);
            void SetRtpTimestamp(uint32_t time);
            void SetSentPacketCount(uint32_t count);
            void SetSentBytes(uint32_t bytes); 
        private:
            uint32_t ssrc_{0};
            uint32_t ntp_mword_{0};
            uint32_t ntp_lword_{0};
            uint32_t rtp_timestamp_{0};
            uint32_t sent_pkt_cnt_{0};
            uint32_t sent_bytes_{0};
        };
    }
}