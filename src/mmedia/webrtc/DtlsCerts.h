#pragma once

#include <cstdint>
#include <string>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>

namespace tmms
{
    namespace mm
    {
        class DtlsCerts
        {
        public:
            DtlsCerts() = default;
            ~DtlsCerts();

            bool Init();
            const std::string &Fingerprint()const;
            EVP_PKEY *GetPrivateKey()const;
            X509 *GetCerts()const;
            uint32_t GenRandom();
        private:
            EVP_PKEY * dtls_pkey_{nullptr};
            X509 * dtls_certs_{nullptr};
            std::string fingerprint_;
        };
    }
}