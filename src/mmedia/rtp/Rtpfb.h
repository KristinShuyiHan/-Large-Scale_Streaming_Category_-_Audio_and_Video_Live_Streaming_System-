#pragma once

#include "Rtcp.h"
#include <cstdint>
#include <set>

namespace tmms
{
    namespace mm
    {
        struct SeqCmp
        {
            int16_t uint16_t_distance(uint16_t prev, uint16_t current)
            {
                return (int16_t)(current - prev);
            }

            bool is_uint16_t_newer(uint16_t prev, uint16_t current)
            {
                return uint16_t_distance(prev, current) > 0;
            }

            bool operator()(uint16_t prev, uint16_t current)
            {
                return is_uint16_t_newer(prev,current);
            }

        };
        class Rtpfb:public Rtcp
        {
        public:
            Rtpfb();
            ~Rtpfb() = default;

            uint32_t Ssrc() const;
            uint32_t MediaSsrc() const;
            const std::set<uint16_t,SeqCmp> &LostSeqs()const;
            size_t DecodeNack(const char *data, size_t len);
            void Decode(const char *data, size_t len) override;
            PacketPtr Encode() override;
            void Print();
        private:
            uint32_t ssrc_{0};
            uint32_t media_ssrc_{0};
            std::set<uint16_t,SeqCmp> lost_seqs_;
        };
    }
}