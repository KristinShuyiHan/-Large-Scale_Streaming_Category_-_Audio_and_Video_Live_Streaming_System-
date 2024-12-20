#pragma once

#include "HttpTypes.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <sstream>

namespace tmms
{
    namespace mm
    {
        class HttpRequest;
        using HttpRequestPtr = std::shared_ptr<HttpRequest>;
        class HttpRequest
        {
        public:
            explicit HttpRequest(bool is_request=true);
            const std::unordered_map<std::string,std::string> &Headers() const;
            void AddHeader(const std::string &field, const std::string &value);
            void RemoveHeader(const std::string &key) ;
            const std::string &GetHeader(const std::string &field) const;
            void AddHeader(std::string &&field, std::string &&value);         
            std::string MakeHeaders();
            void SetQuery(const std::string &query);
            void SetQuery(std::string &&query);
            void SetParameter(const std::string &key, const std::string &value);
            void SetParameter(std::string &&key, std::string &&value);
            const std::string &GetParameter(const std::string &key) const;
            const std::string &Query() const ;

            void SetMethod(const std::string &method);
            void SetMethod(HttpMethod method);
            HttpMethod Method() const;
            void SetVersion(Version v);
            void SetVersion(const std::string &version);
            Version GetVersion() const ;
            void SetPath(const std::string &path);
            const std::string &Path() const;
            void SetStatusCode(int32_t code);
            uint32_t GetStatusCode() const;
            void SetBody(const std::string &body);
            void SetBody(std::string &&body);
            const std::string &Body() const;
            std::string AppendToBuffer();
            bool IsRequest() const;
            bool IsStream() const;
            bool IsChunked() const;
            void SetIsStream(bool s);
            void SetIsChunked(bool c);

            static HttpRequestPtr NewHttp400Response();
            static HttpRequestPtr NewHttp404Response();
            static HttpRequestPtr NewHttpOptionsResponse();
        private:
            void AppendRequestFirstLine(std::stringstream &ss);
            void AppendResponseFirstLine(std::stringstream &ss);

            void ParseParameters();

            HttpMethod method_{kInvalid};
            Version version_{Version::kUnknown};
            std::string path_;
            std::string query_;
            std::unordered_map<std::string,std::string> headers_;
            std::unordered_map<std::string,std::string> parameters_;
            std::string body_;
            uint32_t code_{0};
            bool is_request_{true};
            bool is_stream_{false};
            bool is_chunked_{false};            
        };
    }
}