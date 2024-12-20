#pragma once

#include "PlayerUser.h"
#include "mmedia/webrtc/Sdp.h"
#include "mmedia/webrtc/Dtls.h"
#include "mmedia/webrtc/Srtp.h"
#include "mmedia/rtp/RtpMuxer.h"
#include <string>
#include <cstdint>

namespace tmms
{
    namespace live
    {
        using namespace mm;

        class WebrtcPlayerUser:public PlayerUser,public DtlsHandler
        {
        public:
            explicit WebrtcPlayerUser(const ConnectionPtr &ptr,const StreamPtr &stream,const SessionPtr &s);
            
            bool PostFrames() override;
            UserType GetUserType() const override;

            bool ProcessOfferSdp(const std::string &sdp);
            const std::string &LocalUFrag() const;
            const std::string &LocalPasswd() const;
            const std::string &RemoteUFrag() const;
            std::string BuildAnswerSdp();
            void SetConnection(const ConnectionPtr &conn) override;
            void OnDtlsRecv(const char *buf,size_t size);
            std::shared_ptr<struct sockaddr_in6> GetSockAddr() const
            {
                return addr_;
            }
            void SetSockAddr(const network::InetAddress &addr)
            {
                if(!addr_)
                {
                    addr_ = std::make_shared<struct sockaddr_in6>();
                }
                addr.GetSockAddr((struct sockaddr*)addr_.get());
            }
            void OnRtcp(const char *buf,size_t size);
        private:
            void OnDtlsSend(const char *data,size_t size, Dtls *dtls) override;
            void OnDtlsHandshakeDone(Dtls *dtls) override;      
            static std::string GetUFrag(int size);
            static uint32_t GetSsrc(int size);
            void SendSR(bool is_video);
            void CheckSR();
            void AddVideo(const PacketPtr &pkt);
            void AddAudio(const PacketPtr &pkt);
            PacketPtr GetVideo(int idx);
            PacketPtr GetAudio(int idx); 
            void ProcessRtpfb(const char *buf,size_t size);

            std::string local_ufrag_;
            std::string local_passwd_;
            Sdp sdp_;
            Dtls dtls_;
            PacketPtr packet_;
            std::shared_ptr<struct sockaddr_in6> addr_;
            socklen_t addr_len_{sizeof(struct sockaddr_in6)};
            bool dtls_done_{false};
            Srtp srtp_;
            RtpMuxer rtp_muxer_;
            bool got_key_frame_{false};

            uint32_t video_out_bytes_{0};
            uint32_t audio_out_bytes_{0};
            uint32_t video_out_pkts_count_{0};
            uint32_t audio_out_pkts_count_{0};
            uint64_t video_sr_timestamp_{0};
            uint64_t audio_sr_timestamp_{0};

            std::mutex queue_lock_;
            std::vector<PacketPtr> video_queue_;
            std::vector<PacketPtr> audio_queue_;

            
        };
    }
}