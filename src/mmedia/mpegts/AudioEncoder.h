#pragma once

#include "mmedia/base/AVTypes.h"
#include "mmedia/demux/AudioDemux.h"
#include "mmedia/mpegts/StreamWriter.h"
#include "mmedia/base/Packet.h"
#include <cstdint>
#include <list>

namespace tmms
{
    namespace mm
    {
        class AudioEncoder
        {
        public:
            AudioEncoder() = default;
            ~AudioEncoder() = default;

            int32_t EncodeAudio(StreamWriter* writer, PacketPtr &data,int64_t dts);
            void SetPid(uint16_t pid);
            void SetStreamType (TsStreamType type);

        private:
            int32_t EncodeAAC(StreamWriter* writer,std::list<SampleBuf> &sample_list,int64_t pts);
            int32_t EncodeMP3(StreamWriter* writer,std::list<SampleBuf> &sample_list,int64_t pts);
            int32_t WriteAudioPes(StreamWriter* writer,std::list<SampleBuf> &result, int payload_size,int64_t dts);

            uint16_t pid_{0xe000};
            TsStreamType type_{kTsStreamReserved};
            int8_t cc_{-1};
            AudioDemux demux_;
        };
    }
}