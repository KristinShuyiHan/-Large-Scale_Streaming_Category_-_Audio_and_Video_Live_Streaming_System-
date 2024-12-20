#pragma once
#include "json/json.h"
#include <string>
#include <memory>
#include <vector>

namespace tmms
{
    namespace base
    {
        using std::string;
        class Target;
        using TargetPtr = std::shared_ptr<Target>;

        class DomainInfo;
        class AppInfo
        {
        public:
            explicit AppInfo(DomainInfo &d);

            bool ParseAppInfo(Json::Value &root);

            DomainInfo &domain_info;
            std::string domain_name;
            std::string app_name;
            uint32_t max_buffer{1000};
            bool rtmp_support{false};
            bool flv_support{false};
            bool hls_support{false};
            uint32_t content_latency{3*1000};
            uint32_t stream_idle_time{30*1000};
            uint32_t stream_timeout_time{30*1000};

            std::vector<TargetPtr> pulls;
        };
    }
}