#pragma once

#include "StreamWriter.h"
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        class PSIWriter
        {
        public:
            PSIWriter() = default;
            virtual ~PSIWriter() = default;

            void SetVersion(uint8_t v);
            int WriteSection(StreamWriter * w, int id, int sec_num, int last_sec_num,uint8_t *buf, int len);
        protected:
            void PushSection(StreamWriter * w,uint8_t *buf, int len);
            int8_t cc_{-1};
            uint16_t pid_{0xe000};
            uint8_t table_id_{0x00};
            int8_t version_{0x00};
        };
    }
}