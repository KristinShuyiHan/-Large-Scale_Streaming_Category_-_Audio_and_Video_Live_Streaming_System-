#pragma once

#include <cstdint>
#include <string>

namespace tmms
{
    namespace mm
    {
        class Sdp
        {
        public:
            Sdp() = default;
            ~Sdp() = default;

            bool Decode(const std::string &sdp);
            const std::string &GetRemoteUFrag() const;
            const std::string &GetFingerprint()const;
            int32_t GetVideoPayloadType() const;
            int32_t GetAudioPayloadType() const;

            void SetFingerprint(const std::string &fp);
            void SetStreamName(const std::string &name);
            void SetLocalUFrag(const std::string &frag);
            void SetLocalPasswd(const std::string &pwd);
            void SetServerPort(uint16_t port);
            void SetServerAddr(const std::string &addr);
            void SetVideoSsrc(uint32_t ssrc);
            void SetAudioSsrc(int32_t ssrc);
            const std::string &GetLocalPasswd()const;
            const std::string &GetLocalUFrag()const;
            uint32_t VideoSsrc() const;
            uint32_t AudioSsrc() const;
            std::string Encode();

        private:
            int32_t audio_payload_type_{-1};
            int32_t video_payload_type_{-1};
            std::string remote_ufrag_;
            std::string remote_passwd_;
            std::string local_ufrag_;
            std::string local_passwd_;
            std::string fingerprint_;
            int32_t video_ssrc_{0};
            int32_t audio_ssrc_{0};
            int16_t server_port_{0};
            std::string server_addr_;
            std::string stream_name_;
        };
    }
}