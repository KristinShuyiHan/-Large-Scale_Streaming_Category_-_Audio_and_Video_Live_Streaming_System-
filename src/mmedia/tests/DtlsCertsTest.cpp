#include "mmedia/webrtc/DtlsCerts.h"
#include <iostream>

using namespace tmms::mm;

int main(int argc,const char ** agrv)
{
    DtlsCerts certs;
    certs.Init();
    std::cout << certs.Fingerprint() << std::endl;
    return 0;
}