#include "HttpParser.h"
#include "base/StringUtils.h"
#include "mmedia/base/MMediaLog.h"
#include "HttpUtils.h"
#include <algorithm>

using namespace tmms::mm;
static std::string CRLFCRLF = "\r\n\r\n";
static int32_t kHttpMaxBodySize = 64*1024;
namespace
{
    static std::string string_empty;
}

HttpParserState HttpParser::Parse(MsgBuffer &buf)
{
    if(buf.ReadableBytes() == 0)
    {
        return state_;
    }
    if(state_ == kExpectHttpComplete)
    {
        ClearForNextHttp();
    }
    else if(state_ == kExpectChunkComplete)
    {
        ClearForNextChunk();
    }
    switch (state_)
    {
        case kExpectHeaders:
        {
            if(buf.ReadableBytes()>CRLFCRLF.size())
            {
                auto *space = std::search(buf.Peek(),(const char *)buf.BeginWrite(),CRLFCRLF.data(),CRLFCRLF.data()+CRLFCRLF.size());
                if(space != (const char *)buf.BeginWrite())
                {
                    auto size = space - buf.Peek();
                    header_.assign(buf.Peek(),size);
                    buf.Retrieve(size+4);
                    ParseHeaders();
                    if(state_ == kExpectHttpComplete||state_ == kExpectError)
                    {
                        return state_;
                    }
                }
                else
                {
                    if(buf.ReadableBytes() > kHttpMaxBodySize)
                    {
                        reason_ = k400BadRequest;
                        state_ = kExpectError;
                        return state_;
                    }
                    return kExpectContinue;
                }
            }
            else 
            {
                return kExpectContinue;
            }
        }
        break;
        case kExpectNormalBody:
        {
            ParseNormalBody(buf);
            break;
        }
        case kExpectStreamBody:
        {
            ParseStream(buf);
            break;
        }
        case kExpectChunkLen:
        {
            auto crlf = buf.FindCRLF();
            if(crlf)
            {
                std::string len(buf.Peek(),crlf);
                char *end;
                current_chunk_length_ = std::strtol(len.c_str(),&end,16);
                HTTP_DEBUG << " chunk len:" << current_chunk_length_;
                if(current_chunk_length_>1024*1024)
                {
                    HTTP_ERROR << "error chunk len.";
                    state_ = kExpectError;
                    reason_ = k400BadRequest;
                }
                buf.RetrieveUntil(crlf+2);
                if(current_chunk_length_ == 0)
                {
                    state_ = kExpectLastEmptyChunk;
                }
                else 
                {
                    current_chunk_length_ += 2;
                    state_ = kExpectChunkBody;
                }
            }
            else
            {
                if(buf.ReadableBytes() > 32)
                {
                    buf.RetrieveAll();
                    reason_ = k400BadRequest;
                    state_ = kExpectError;
                    return state_;
                }
            }
            break;
        }
        case kExpectChunkBody:
        {
            ParseChunk(buf);
            if(state_ == kExpectChunkComplete)
            {
                return state_;
            }
            break;
        }
        case kExpectLastEmptyChunk:
        {
            auto crlf = buf.FindCRLF();
            if(crlf)
            {
                buf.RetrieveUntil(crlf+2);
                chunk_.reset();
                state_ = kExpectChunkComplete;
                break;
            }
        }
        default:
            break;
    }
    return state_;
}
void HttpParser::ParseStream(MsgBuffer &buf)
{
    if(!chunk_)
    {
        chunk_ = Packet::NewPacket(kHttpMaxBodySize);
    }
    auto size = std::min((int)buf.ReadableBytes(),chunk_->Space());
    memcpy(chunk_->Data()+chunk_->PacketSize(),buf.Peek(),size);
    chunk_->UpdatePacketSize(size);
    buf.Retrieve(size);

    if(chunk_->Space() == 0)
    {
        state_ = kExpectChunkComplete;
    }
}
void HttpParser::ParseNormalBody(MsgBuffer &buf)
{
    if(!chunk_)
    {
        chunk_ = Packet::NewPacket(current_content_length_);
    }
    auto size = std::min((int)buf.ReadableBytes(),chunk_->Space());
    memcpy(chunk_->Data()+chunk_->PacketSize(),buf.Peek(),size);
    chunk_->UpdatePacketSize(size);
    buf.Retrieve(size);
    current_content_length_ -= size;
    if(current_content_length_ == 0)
    {
        state_ = kExpectHttpComplete;
    }
}
void HttpParser::ParseChunk(MsgBuffer &buf)
{
    if(!chunk_)
    {
        chunk_ = Packet::NewPacket(current_chunk_length_);
    }
    auto size = std::min((int)buf.ReadableBytes(),chunk_->Space());
    memcpy(chunk_->Data()+chunk_->PacketSize(),buf.Peek(),size);
    chunk_->UpdatePacketSize(size);
    buf.Retrieve(size);
    current_chunk_length_ -= size;
    if(current_chunk_length_ == 0||chunk_->Space() == 0)
    {
        chunk_->SetPacketSize(chunk_->PacketSize() - 2);
        state_ = kExpectChunkComplete;
    }
}
void HttpParser::ParseHeaders()
{
    auto list = base::StringUtils::SplitString(header_,"\r\n");
    if(list.size()<1)
    {
        reason_ = k400BadRequest;
        state_ = kExpectError;
        return;
    }
    ProcessMethodLine(list[0]);

    for(auto &l:list)
    {
        auto pos = l.find_first_of(':');
        if(pos !=std::string::npos)
        {
            std::string k = l.substr(0,pos);
            std::string v = l.substr(pos+1);
            k = HttpUtils::Trim(k);
            v = HttpUtils::Trim(v);
            HTTP_DEBUG << "parse header k:" << k << " v:" << v;
            req_->AddHeader(std::move(k),std::move(v));
        }
    }
    auto len = req_->GetHeader("content-length");
    if(!len.empty())
    {
        HTTP_TRACE << "content-length:" << len;
        try
        {
            current_content_length_ = std::stoull(len);
        }
        catch(...)
        {
            reason_ = k400BadRequest;
            state_ = kExpectError;
            return;
        }

        if(current_content_length_ == 0)
        {
            state_ = kExpectHttpComplete;
        }
        else
        {
            state_ = kExpectNormalBody;
        }
    }
    else 
    {
        const std::string &chunk = req_->GetHeader("transfer-encoding");
        if(!chunk.empty()&&chunk == "chunked")
        {
            is_chunked_ = true;
            req_->SetIsChunked(true);
            state_ = kExpectChunkLen;
        }
        else 
        {
            if((!is_request_&&req_->GetStatusCode()!=200)
                ||(is_request_
                    &&(req_->Method() == kGet
                        || req_->Method() == kHead 
                        || req_->Method() == kOptions)))
            {
                current_chunk_length_ = 0;
                state_ = kExpectHttpComplete;
            }
            else 
            {
                current_content_length_ = -1;
                is_stream_ = true;
                req_->SetIsStream(true);
                state_ = kExpectStreamBody;
            }
        }
    }
}
void HttpParser::ProcessMethodLine(const std::string &line)
{
    HTTP_DEBUG << "parse method line:" << line;
    auto list = base::StringUtils::SplitString(line," ");
    std::string str = list[0];
    std::transform(str.begin(),str.end(),str.begin(),::tolower);
    if(str[0] == 'h'&&str[1] == 't'&& str[2] == 't'&&str[3] == 'p')
    {
        is_request_ = false;
    }
    else
    {
        is_request_ = true;
    }
    
    if(req_)
    {
        req_.reset();
    }
    req_ = std::make_shared<HttpRequest>(is_request_);
    if(is_request_)
    {
        req_->SetMethod(list[0]);
        const std::string &path = list[1];
        auto pos = path.find_first_of("?");
        if(pos != std::string::npos)
        {
            req_->SetPath(path.substr(0,pos));
            req_->SetQuery(path.substr(pos+1));
        }
        else 
        {
            req_->SetPath(path);
        }
        req_->SetVersion(list[2]);
        HTTP_DEBUG << "http method:" << list[0] 
                << " path:" << req_->Path() 
                << " query:" << req_->Query() 
                << " version:" << list[2];
    }
    else
    {
        req_->SetVersion(list[0]);
        req_->SetStatusCode(std::atoi(list[1].c_str()));
        HTTP_DEBUG << "http code:" << list[1] 
                << " version:" << list[0];        
    }
}

const PacketPtr &HttpParser::Chunk() const
{
    return chunk_;
}
HttpStatusCode HttpParser::Reason() const
{
    return reason_;
}
void HttpParser::ClearForNextHttp()
{
    state_ = kExpectHeaders;
    header_.clear();
    req_.reset();
    current_content_length_ = -1;
    chunk_.reset();
}
void HttpParser::ClearForNextChunk()
{
    if(is_chunked_)
    {
        state_ = kExpectChunkLen;
        current_chunk_length_ = -1;
    }
    else
    {
        if(is_stream_)
        {
            state_ = kExpectStreamBody;
        }
        else 
        {
            state_ = kExpectHeaders;
            current_chunk_length_ = -1;
        }
    }
    chunk_.reset();
}