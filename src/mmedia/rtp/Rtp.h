#pragma once

#include "mmedia/base/Packet.h"
#include "mmedia/base/AVTypes.h"
#include <cstdint>
#include <string>
#include <list>
namespace tmms
{
    namespace mm
    {
        const int32_t kRtpMaxPayloadSize = 1400;
        class Rtp
        {
        public:
            Rtp(int32_t pt);
            ~Rtp()=default;

            virtual bool Encode(std::list<SampleBuf> &ins,uint32_t ts, std::list<PacketPtr> &outs) = 0;
            void SetMarker(bool on);
            void SetSequenceNumber(uint16_t);
            uint16_t SequenceNumber() const;
            void SetTimestamp(uint32_t timestamp);
            uint32_t Timestamp()const;
            void SetSsrc(uint32_t ssrc);
            uint32_t Ssrc()const;
            void SetSample(int32_t s);
        protected:
            void EncodeHeader(char *buf);
            int32_t HeaderSize() const;

            uint32_t version_:2;
            uint32_t padding_:1;
            uint32_t extern_:1;
            uint32_t csrc_count_:4;
            uint32_t marker_:1;
            uint32_t payload_type_:7;
            uint32_t sequence_{0};
            uint32_t timestamp_{0};
            uint32_t ssrc_{0};
            uint32_t sample_{0};
        };
    }
}