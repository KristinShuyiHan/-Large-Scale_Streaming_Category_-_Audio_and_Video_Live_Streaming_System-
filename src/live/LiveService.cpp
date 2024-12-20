#include "LiveService.h"
#include "base/StringUtils.h"
#include "live/base/LiveLog.h"
#include "mmedia/rtmp/RtmpServer.h"
#include "mmedia/http/HttpServer.h"
#include "mmedia/http/HttpRequest.h"
#include "mmedia/http/HttpUtils.h"
#include "mmedia/http/HttpContext.h"
#include "base/Config.h"
#include "Session.h"
#include "base/TTime.h"
#include "Stream.h"
#include "network/DnsService.h"
#include "mmedia/flv/FlvContext.h"
#include "live/WebrtcService.h"
#include "mmedia/webrtc/WebrtcServer.h"
#include "mmedia/webrtc/Srtp.h"

using namespace tmms::live;
using namespace tmms::mm;

namespace
{
    static SessionPtr session_null;
}
SessionPtr LiveService::CreateSession(const std::string &session_name)
{
    std::lock_guard<std::mutex> lk(lock_);
    auto iter = sessions_.find(session_name);
    if(iter != sessions_.end())
    {
        return iter->second;
    }

    auto list = base::StringUtils::SplitString(session_name,"/");
    if(list.size() != 3)
    {
        LIVE_ERROR << "create session failed. Invalid session name:" << session_name;
        return session_null;
    }
    ConfigPtr config = sConfigMgr->GetConfig();
    auto app_info = config->GetAppInfo(list[0],list[1]);
    if(!app_info)
    {
        LIVE_ERROR << "create session failed. cant found config. domain:" << list[0] << " app:" << list[1];
        return session_null;
    }
    auto s = std::make_shared<Session>(session_name);
    s->SetAppInfo(app_info);

    sessions_.emplace(session_name,s);
    LIVE_DEBUG << "create session success. session_name:" << session_name << " now:" << base::TTime::NowMS();
    return s;
}
SessionPtr LiveService::FindSession(const std::string &session_name)
{
    std::lock_guard<std::mutex> lk(lock_);
    auto iter = sessions_.find(session_name);
    if(iter != sessions_.end())
    {
        return iter->second;
    }
    return session_null;
}
bool LiveService::CloseSession(const std::string &session_name)
{
    SessionPtr s;
    {
        std::lock_guard<std::mutex> lk(lock_);
        auto iter = sessions_.find(session_name);
        if(iter != sessions_.end())
        {
            s = iter->second;
            sessions_.erase(iter);
        }
    }
    if(s)
    {
        LIVE_INFO << " close session:" << s->SessionName()  << " now:" << base::TTime::NowMS();
        s->Clear();
    }
    return true;
}
void LiveService::OnTimer(const TaskPtr &t)
{
    std::lock_guard<std::mutex> lk(lock_);
    for(auto iter = sessions_.begin();iter != sessions_.end();)
    {
        if(iter->second->IsTimeout())
        {
            LIVE_INFO << "session:" << iter->second->SessionName() 
                    << " is timeout. close it. Now:" << base::TTime::NowMS();
            iter->second->Clear();
            iter = sessions_.erase(iter);
        }
        else
        {
            iter ++;
        }
    }
    t->Restart();
}

void LiveService::OnNewConnection(const TcpConnectionPtr &conn)
{

}
void LiveService::OnConnectionDestroy(const TcpConnectionPtr &conn)
{
    auto user = conn->GetContext<User>(kUserContext);
    if(user)
    {
        user->GetSession()->CloseUser(user);
        conn->ClearContext(kUserContext);
    }
}
void LiveService::OnActive(const ConnectionPtr &conn)
{
    auto user = conn->GetContext<PlayerUser>(kUserContext);
    if(user&&user->GetUserType()>=UserType::kUserTypePlayerPav)
    {
        user->PostFrames();
    }
    // else
    // {
    //     LIVE_ERROR << "no user found.host:" << conn->PeerAddr().ToIpPort();
    //     conn->ForceClose();
    // }
}

bool LiveService::OnPlay(const TcpConnectionPtr &conn,
                        const std::string &session_name, 
                        const std::string &param)
{
    LIVE_DEBUG << "on play session name:" << session_name 
                << " param:" << param 
                << " host:" << conn->PeerAddr().ToIpPort()
                << " now:" << TTime::NowMS();
    auto s = CreateSession(session_name);
    if(!s)
    {
        LIVE_ERROR << "create session failed.session name:" << session_name;
        conn->ForceClose();
        return false;
    } 
    auto user = s->CreatePlayerUser(conn,session_name,param,UserType::kUserTypePlayerRtmp);
    if(!user)
    {
        LIVE_ERROR << "create user failed.session name:" << session_name;
        conn->ForceClose();
        return false;
    }     
    conn->SetContext(kUserContext,user);     
    s->AddPlayer(std::dynamic_pointer_cast<PlayerUser>(user));
    return true;     
}
bool LiveService::OnPublish(const TcpConnectionPtr &conn,
                        const std::string &session_name, 
                        const std::string &param)
{
    LIVE_DEBUG << "on publish session name:" << session_name 
                << " param:" << param 
                << " host:" << conn->PeerAddr().ToIpPort()
                << " now:" << TTime::NowMS();
    auto s = CreateSession(session_name);
    if(!s)
    {
        LIVE_ERROR << "create session failed.session name:" << session_name;
        conn->ForceClose();
        return false;
    } 
    auto user = s->CreatePublishUser(conn,session_name,param,UserType::kUserTypePublishRtmp);
    if(!user)
    {
        LIVE_ERROR << "create user failed.session name:" << session_name;
        conn->ForceClose();
        return false;
    }     
    conn->SetContext(kUserContext,user);
    s->SetPublisher(user);
    return true;
}
void LiveService::OnRecv(const TcpConnectionPtr &conn ,PacketPtr &&data) 
{
    auto user = conn->GetContext<User>(kUserContext);
    if(!user)
    {
        LIVE_ERROR << "no found user. host:" << conn->PeerAddr().ToIpPort();
        conn->ForceClose();
        return ;
    }
    user->GetStream()->AddPacket(std::move(data));
}
void LiveService::OnSent(const TcpConnectionPtr &conn)
{

}
bool LiveService::OnSentNextChunk(const TcpConnectionPtr &conn)
{
    return false;
}
void LiveService::OnRequest(const TcpConnectionPtr &conn,const HttpRequestPtr &req,const PacketPtr &packet)
{
    auto http_cxt = conn->GetContext<HttpContext>(kHttpContext);
    if(!http_cxt)
    {
        LIVE_ERROR << "no found http context.something must be wrong.";
        return;
    }
    if(req->IsRequest())
    {
        LIVE_DEBUG << "req method:" << req->Method() << " path:" << req->Path();
    }
    else 
    {
        LIVE_DEBUG << "req code:" << req->GetStatusCode() << " msg:" << HttpUtils::ParseStatusMessage(req->GetStatusCode());
    }

    auto headers = req->Headers();
    for(auto const &h:headers)
    {
        LIVE_DEBUG << h.first << ":" << h.second;
    }
    
    if(req->IsRequest())
    {
        //http://ip:port/domain/app/stream.flv
        //http://ip:port/domain/app/stream/filename.flv
        auto list = base::StringUtils::SplitString(req->Path(),"/");
        if(list.size()<4)
        {
            auto res = HttpRequest::NewHttp400Response();
            http_cxt->PostRequest(res);
            return ;
        }
        const std::string &domain = list[1];
        const std::string &app = list[2];
        string filename = list[3];
        std::string stream_name;
        if(list.size()>4)
        {
            filename = list[4];
            stream_name = list[3];
        }
        else 
        {
            stream_name = base::StringUtils::FileName(filename);
        }

        std::string session_name = domain+"/"+app+"/"+stream_name;
        LIVE_DEBUG << "request session name:" << session_name;
        auto s = CreateSession(session_name);
        if(!s)
        {
            LIVE_ERROR << "cant create session  name:" << session_name;
            auto http_cxt = conn->GetContext<HttpContext>(kHttpContext);
            if(http_cxt)
            {
                auto res = HttpRequest::NewHttp404Response();
                http_cxt->PostRequest(res);
                return ;
            }
        }        
        std::string ext = base::StringUtils::Extension(filename);
        if(ext == "flv")
        {
            auto user = s->CreatePlayerUser(conn,session_name,"",UserType::kUserTypePlayerFlv);
            if(!user)   
            {
                LIVE_ERROR << "cant create user  session name:" << session_name;
                auto res = HttpRequest::NewHttp404Response();
                http_cxt->PostRequest(res);
                return ;  
            }    
            conn->SetContext(kUserContext,user);
            auto flv = std::make_shared<FlvContext>(conn,this);
            conn->SetContext(kFlvContext,flv);
            s->AddPlayer(std::dynamic_pointer_cast<PlayerUser>(user));     
        }
        else if(ext == "m3u8")
        {
            auto playlist = s->GetStream()->PlayList();
            if(!playlist.empty())
            {
                auto res = std::make_shared<HttpRequest>(false);
                res->AddHeader("server","tmms");
                res->AddHeader("content-length",std::to_string(playlist.size()));
                res->AddHeader("content-type","application/vnd.apple.mpegurl");
                res->SetStatusCode(200);
                res->SetBody(playlist);
                LIVE_DEBUG << "http:\n" << res->AppendToBuffer();
                http_cxt->PostRequest(res);
            }
            else 
            {
                auto res = HttpRequest::NewHttp404Response();
                http_cxt->PostRequest(res);
                return ;  
            }
        }
        else if(ext == "ts")
        {
            LIVE_DEBUG << "request ts:" << filename;
            auto frag = s->GetStream()->GetFragment(filename);
            if(frag)
            {
                auto res = std::make_shared<HttpRequest>(false);
                res->AddHeader("server","tmms");
                res->AddHeader("content-length",std::to_string(frag->Size()));
                res->AddHeader("content-type","video/MP2T");
                res->SetStatusCode(200);
                http_cxt->PostRequest(res->MakeHeaders(),frag->FragmentData());
            }
        }
    }
}
void LiveService::Start()
{
    ConfigPtr config = sConfigMgr->GetConfig();
    pool_ = new EventLoopThreadPool(config->thread_nums_,config->cpu_start_,config->cpus_);
    pool_->Start();

    sDnsService->Start();
    // 
    auto services = config->GetServiceInfos();
    auto eventloops = pool_->GetLoops();
    LIVE_TRACE << "eventloops size:" << eventloops.size();
    for(auto &el:eventloops)
    {
        for(auto &s:services)
        {
            if(s->protocol == "RTMP"||s->protocol == "rtmp")
            {
                InetAddress local(s->addr,s->port);
                TcpServer *server = new RtmpServer(el,local,this);
                servers_.push_back(server);
                servers_.back()->Start();
            }
            else if(s->protocol == "HTTP"||s->protocol == "http")
            {
                if(s->transport == "webrtc"||s->transport == "WEBRTC")
                {
                    InetAddress local(s->addr,s->port);
                    TcpServer *server = new HttpServer(el,local,sWebrtcService);
                    servers_.push_back(server);
                    servers_.back()->Start();
                }
                else
                {
                    InetAddress local(s->addr,s->port);
                    TcpServer *server = new HttpServer(el,local,this);
                    servers_.push_back(server);
                    servers_.back()->Start();
                }
            } 
            else if(s->protocol == "webrtc"||s->protocol == "WEBRTC")
            {
                if(s->transport == "udp"||s->transport == "UDP")
                {
                    if(!webrtc_server_)
                    {
                        Srtp::InitSrtpLibrary();
                        InetAddress local(s->addr,s->port);
                        webrtc_server_ =std::make_shared<WebrtcServer>(el,local,sWebrtcService);
                        webrtc_server_->Start();
                    }
                }
            }
        }
    }
    
    TaskPtr t = std::make_shared<Task>(std::bind(&LiveService::OnTimer,this,std::placeholders::_1),5000);
    sTaskMgr->Add(t);
}

void LiveService::Stop()
{

}
EventLoop *LiveService::GetNextLoop()
{
    return pool_->GetNextLoop();
}

