#pragma once

#include "mmedia/base/Packet.h"
#include "mmedia/mpegts/StreamWriter.h"
#include <string>
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        const int32_t kFragmentStepSize = 128*1024;

        class Fragment:public StreamWriter
        {
        public:
            Fragment() = default;
            ~Fragment() = default;

            void AppendTimeStamp(int64_t dts) override;
            int32_t Write(void* buf, uint32_t size) override;
            int32_t Size() override;
            char* Data() override;

            int64_t Duration() const;
            const std::string &FileName() const;
            void SetBaseFileName(const std::string &v);
            int32_t SequenceNo() const;
            void SetSequenceNo(int32_t no);
            void Reset();
            PacketPtr &FragmentData();
            void Save();
        private:
            int64_t duration_{0};
            std::string filename_;
            int64_t start_dts_{-1};
            PacketPtr data_;
            int32_t buf_size_{512*1024};
            int32_t data_size_{0};
            int32_t sequence_no_{0};
        };
    }
}