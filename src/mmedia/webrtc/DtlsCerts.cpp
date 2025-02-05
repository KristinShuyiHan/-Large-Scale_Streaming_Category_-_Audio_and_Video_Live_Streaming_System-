#include "DtlsCerts.h"
#include "mmedia/base/MMediaLog.h"
#include <random>

using namespace tmms::mm;

DtlsCerts::~DtlsCerts()
{
    if(dtls_pkey_)
    {
        EVP_PKEY_free(dtls_pkey_);
        dtls_pkey_ = nullptr;
    }

    if(dtls_certs_)
    {
        X509_free(dtls_certs_);
        dtls_certs_ = nullptr;
    }
}
bool DtlsCerts::Init()
{
    dtls_pkey_ = EVP_PKEY_new();
    if(!dtls_pkey_)
    {
        WEBRTC_ERROR << "EVP_PKEY_new failed.";
        return false;
    }

    EC_KEY * key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if(!key)
    {
        WEBRTC_ERROR << "EC_KEY_new_by_curve_name failed.";
        return false;
    }
    EC_KEY_set_asn1_flag(key,OPENSSL_EC_NAMED_CURVE);

    auto ret = EC_KEY_generate_key(key);
    if(!ret)
    {
        WEBRTC_ERROR << "EC_KEY_generate_key failed.";
        return false;
    }

    EVP_PKEY_assign_EC_KEY(dtls_pkey_,key);

    dtls_certs_ = X509_new();
    auto req = GenRandom();
    ASN1_INTEGER_set(X509_get_serialNumber(dtls_certs_),req);
    X509_gmtime_adj(X509_get_notBefore(dtls_certs_),0);
    X509_gmtime_adj(X509_get_notAfter(dtls_certs_),60*60*24*356);

    X509_NAME * subject =  X509_NAME_new();

    const std::string  &aor = "tmms.net" + std::to_string(req);
    X509_NAME_add_entry_by_txt(subject,"O",MBSTRING_ASC,(const unsigned char*)aor.c_str(),aor.size(),-1,0);
    
    const std::string  &aor1 = "tmms.cn";
    X509_NAME_add_entry_by_txt(subject,"CN",MBSTRING_ASC,(const unsigned char*)aor1.c_str(),aor1.size(),-1,0);  

    X509_set_issuer_name(dtls_certs_,subject);
    X509_set_subject_name(dtls_certs_,subject);

    X509_set_pubkey(dtls_certs_,dtls_pkey_);

    X509_sign(dtls_certs_,dtls_pkey_,EVP_sha1());

    unsigned char fingerprint_bin[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    X509_digest(dtls_certs_,EVP_sha256(),fingerprint_bin,&len);
    
    char fingerprint_result[(EVP_MAX_MD_SIZE*3)+1];
    for(int i = 0;i<len;i++)
    {
        sprintf(fingerprint_result+(i*3),"%.2X:",fingerprint_bin[i]);
    }
    if(len>0)
    {
        fingerprint_result[(len*3)-1] = 0; 
    }

    fingerprint_.assign(fingerprint_result,(len*3)-1);
    WEBRTC_DEBUG << "fingerprint:" << fingerprint_;
    return true;
}
const std::string &DtlsCerts::Fingerprint()const
{
    return fingerprint_;
}
EVP_PKEY *DtlsCerts::GetPrivateKey()const
{
    return dtls_pkey_;
}
X509 *DtlsCerts::GetCerts()const
{
    return dtls_certs_;
}
uint32_t DtlsCerts::GenRandom()
{
    std::mt19937 mt{std::random_device{}()};
    return mt();
}