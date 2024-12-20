#include "HttpServer.h"
#include "mmedia/base/MMediaLog.h"
#include "HttpContext.h"
#include "mmedia/flv/FlvContext.h"

using namespace tmms::mm;
using HttpContextPtr = std::shared_ptr<HttpContext>;
using FlvContextPtr = std::shared_ptr<FlvContext>;
HttpServer::HttpServer(EventLoop *loop,const InetAddress &local,HttpHandler *handler)
:TcpServer(loop,local),http_handler_(handler)
{

}
HttpServer::~HttpServer()
{
    Stop();
}

void HttpServer::Start()
{
    TcpServer::SetActiveCallback(std::bind(&HttpServer::OnActive,this,std::placeholders::_1));
    TcpServer::SetDestroyConnectionCallback(std::bind(&HttpServer::OnDestroyed,this,std::placeholders::_1));
    TcpServer::SetNewConnectionCallback(std::bind(&HttpServer::OnNewConnection,this,std::placeholders::_1));
    TcpServer::SetWriteCompleteCallback(std::bind(&HttpServer::OnWriteComplete,this,std::placeholders::_1));
    TcpServer::SetMessageCallback(std::bind(&HttpServer::OnMessage,this,std::placeholders::_1,std::placeholders::_2));
    TcpServer::Start();
    HTTP_DEBUG << "HttpServer Start";
}
void HttpServer::Stop()
{
    TcpServer::Stop();
}

void HttpServer::OnNewConnection(const TcpConnectionPtr &conn)
{
    if(http_handler_)
    {
        http_handler_->OnNewConnection(conn);
    }
    HttpContextPtr shake = std::make_shared<HttpContext>(conn,http_handler_);
    conn->SetContext(kHttpContext,shake);
}
void HttpServer::OnDestroyed(const TcpConnectionPtr &conn)
{
    if(http_handler_)
    {
        http_handler_->OnConnectionDestroy(conn);
    }
    conn->ClearContext(kHttpContext);
}
void HttpServer::OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf)
{
    HttpContextPtr shake = conn->GetContext<HttpContext>(kHttpContext);
    if(shake)
    {
        int ret = shake->Parse(buf);
        if(ret == -1)
        {
            conn->ForceClose();
        }
    }
}
void HttpServer::OnWriteComplete(const ConnectionPtr &conn)
{
    HttpContextPtr shake = conn->GetContext<HttpContext>(kHttpContext);
    if(shake)
    {
        shake->WriteComplete(std::dynamic_pointer_cast<TcpConnection>(conn));
    }
    FlvContextPtr flv = conn->GetContext<FlvContext>(kFlvContext);
    if(flv)
    {
        flv->WriteComplete(std::dynamic_pointer_cast<TcpConnection>(conn));
    }    
}
void HttpServer::OnActive(const ConnectionPtr &conn)
{
    if(http_handler_)
    {
        http_handler_->OnActive(conn);
    }
}