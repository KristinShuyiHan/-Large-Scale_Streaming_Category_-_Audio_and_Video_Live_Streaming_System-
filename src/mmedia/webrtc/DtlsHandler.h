#pragma once

#include <cstdint>
#include <cstddef>

namespace tmms
{
    namespace mm
    {
        class Dtls;

        class DtlsHandler
        {
        public:
            virtual void OnDtlsSend(const char *data,size_t size, Dtls *dtls) {}
            virtual void OnDtlsHandshakeDone(Dtls *dtls) {}
        };
    }
}
