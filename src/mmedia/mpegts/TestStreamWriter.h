#pragma once

#include "StreamWriter.h"
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        class TestStreamWriter:public StreamWriter
        {
        public:
            TestStreamWriter();
            ~TestStreamWriter();
            
            void AppendTimeStamp(int64_t pts) override{};
            int32_t Write(void* buf, uint32_t size)override;
            char* Data() override{return nullptr;};
            int Size() override{return 0;};
        private:
            int fd_{-1};            
        }; 

    }
}