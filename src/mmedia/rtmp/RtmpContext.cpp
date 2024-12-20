#include "RtmpContext.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "mmedia/base/BytesWriter.h"
#include "mmedia/rtmp/amf/AMFObject.h"
#include "base/StringUtils.h"
using namespace tmms::mm;

RtmpContext::RtmpContext(const TcpConnectionPtr &conn,RtmpHandler *handler,bool client)
:handshake_(conn,client),connection_(conn),rtmp_handler_(handler),is_client_(client)
{
    commands_["connect"] = std::bind(&RtmpContext::HandleConnect,this,std::placeholders::_1);
    commands_["createStream"] = std::bind(&RtmpContext::HandleCreateStream,this,std::placeholders::_1);
    commands_["_result"] = std::bind(&RtmpContext::HandleResult,this,std::placeholders::_1);
    commands_["_error"] = std::bind(&RtmpContext::HandleError,this,std::placeholders::_1);
    commands_["onStatus"] = std::bind(&RtmpContext::HandleStatus,this,std::placeholders::_1);
    commands_["play"] = std::bind(&RtmpContext::HandlePlay,this,std::placeholders::_1);
    commands_["publish"] = std::bind(&RtmpContext::HandlePublish,this,std::placeholders::_1);
    out_current_ = out_buffer_;
}
int32_t RtmpContext::Parse(MsgBuffer &buf)
{
    int32_t ret = 0;
    if(state_==kRtmpHandShake)
    {
        ret = handshake_.HandShake(buf);
        if(ret == 0)
        {
            state_ = kRtmpMessage;
            if(is_client_)
            {
                SendConnect();
            }
            if(buf.ReadableBytes()>0)
            {
                return Parse(buf);
            }
        }
        else if(ret == -1)
        {
            RTMP_ERROR << "rtmp handshake error. ";
        }
        else if(ret == 2)
        {
            state_ = kRtmpWatingDone;
        }
    }
    else if(state_ == kRtmpMessage)
    {
        auto r = ParseMessage(buf);
        last_left_ = buf.ReadableBytes();
        return r;
    }
    return ret;
}
void RtmpContext::OnWriteComplete()
{
    if(state_ == kRtmpHandShake)
    {
        handshake_.WriteComplete();
    }
    else if(state_ == kRtmpWatingDone)
    {
        state_ = kRtmpMessage;
        if(is_client_)
        {
            SendConnect();
        }
    }
    else if(state_ == kRtmpMessage)
    {
        CheckAndSend();
    }
}
void RtmpContext::StartHandShake()
{
    handshake_.Start();
}

int32_t RtmpContext::ParseMessage(MsgBuffer &buf)
{
    uint8_t fmt;
    uint32_t csid,msg_len=0,msg_sid=0;
    uint8_t msg_type = 0;
    uint32_t total_bytes = buf.ReadableBytes();
    int32_t parsed = 0;

    in_bytes_ += (buf.ReadableBytes()-last_left_);
    SendBytesRecv();
    
    while(total_bytes>1)
    {
        const char *pos = buf.Peek();
        parsed = 0;
        //Basic Header
        fmt = (*pos>>6)&0x03;
        csid = *pos&0x3F;
        parsed++;

        if(csid == 0)
        {
            if(total_bytes<2)
            {
                return 1;
            }
            csid = 64;
            csid += *((uint8_t*)(pos+parsed));
            parsed++;
        }
        else if( csid == 1)
        {
            if(total_bytes<3)
            {
                return 1;
            }
            csid = 64;
            csid += *((uint8_t*)(pos+parsed));
            parsed++;
            csid +=  *((uint8_t*)(pos+parsed))*256;
            parsed ++;           
        }

        int size = total_bytes - parsed;
        if(size == 0
            ||(fmt==0&&size<11)
            ||(fmt==1&&size<7)
            ||(fmt==2&&size<3))
        {
            return 1;
        }

        msg_len = 0;
        msg_sid = 0;
        msg_type = 0;
        int32_t ts = 0;

        RtmpMsgHeaderPtr &prev = in_message_headers_[csid];
        if(!prev)
        {
            prev = std::make_shared<RtmpMsgHeader>();
        }
        msg_len = prev->msg_len;
        if(fmt == kRtmpFmt0 || fmt == kRtmpFmt1)
        {
            msg_len = BytesReader::ReadUint24T((pos+parsed)+3);
        }
        else if(msg_len == 0)
        {
            msg_len = in_chunk_size_;
        }
        PacketPtr &packet = in_packets_[csid];
        if(!packet)
        {
            packet = Packet::NewPacket(msg_len);
            RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
            header->cs_id = csid;
            header->msg_len = msg_len;
            header->msg_sid = msg_sid;
            header->msg_type = msg_type;
            header->timestamp = 0;  
            packet->SetExt(header);          
        }

        RtmpMsgHeaderPtr header = packet->Ext<RtmpMsgHeader>();

        if(fmt == kRtmpFmt0)
        {
            ts = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            in_deltas_[csid] = 0;
            header->timestamp = ts;
            header->msg_len = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            header->msg_type = BytesReader::ReadUint8T(pos+parsed);
            parsed += 1;
            memcpy(&header->msg_sid,pos+parsed,4);
            parsed += 4;
        }
        else if(fmt == kRtmpFmt1)
        {
            ts = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            in_deltas_[csid] = ts;
            header->timestamp = ts + prev->timestamp;
            header->msg_len = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            header->msg_type = BytesReader::ReadUint8T(pos+parsed);
            parsed += 1;
            header->msg_sid = prev->msg_sid;
        }
        else if(fmt == kRtmpFmt2)
        {
            ts = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            in_deltas_[csid] = ts;
            header->timestamp = ts + prev->timestamp;
            header->msg_len = prev->msg_len;
            header->msg_type = prev->msg_type;
            header->msg_sid = prev->msg_sid;
        }    
        else if(fmt == kRtmpFmt3)
        {
            if(header->timestamp == 0)
            {
                header->timestamp = in_deltas_[csid] + prev->timestamp;
            }
            header->msg_len = prev->msg_len;
            header->msg_type = prev->msg_type;
            header->msg_sid = prev->msg_sid;
        } 

        bool ext = (ts == 0xFFFFFF);
        if(fmt == kRtmpFmt3)
        {
            ext = in_ext_[csid];
        }
        in_ext_[csid] = ext;
        if(ext)
        {
            if(total_bytes - parsed < 4)
            {
                return 1;
            }
            ts = BytesReader::ReadUint32T(pos+parsed);
            parsed += 4;
            if(fmt != kRtmpFmt0)
            {
                header->timestamp = ts+ prev->timestamp;
                in_deltas_[csid] = ts;
            }
        }

        int bytes = std::min(packet->Space(),in_chunk_size_);
        if(total_bytes - parsed < bytes)
        {
            return 1;
        }

        const char * body = packet->Data() + packet->PacketSize();
        memcpy((void*)body,pos+parsed,bytes);
        packet->UpdatePacketSize(bytes);
        parsed += bytes;
        
        buf.Retrieve(parsed);
        total_bytes -= parsed;

        prev->cs_id = header->cs_id;
        prev->msg_len = header->msg_len;
        prev->msg_sid = header->msg_sid;
        prev->msg_type = header->msg_type;
        prev->timestamp = header->timestamp;

        if(packet->Space() == 0)
        {
            packet->SetPacketType(header->msg_type);
            packet->SetTimeStamp(header->timestamp);
            MessageComplete(std::move(packet));
            packet.reset();
        }
    }
    return 1;
}
void RtmpContext::SetPacketType(PacketPtr &packet)
{
    if(packet->PacketType() == kRtmpMsgTypeAudio)
    {
        packet->SetPacketType(kPacketTypeAudio);
    }
    else if(packet->PacketType() == kRtmpMsgTypeVideo)
    {
        packet->SetPacketType(kPacketTypeVideo);
    } 
    else if(packet->PacketType() == kRtmpMsgTypeAMFMeta)
    {
        packet->SetPacketType(kPacketTypeMeta);
    }     
    else if(packet->PacketType() == kRtmpMsgTypeAMF3Meta)
    {
        packet->SetPacketType(kPacketTypeMeta3);
    }              
}
void RtmpContext::MessageComplete(PacketPtr && data)
{
    auto type = data->PacketType();
    switch(type)
    {
        case kRtmpMsgTypeChunkSize:
        {
            HandleChunkSize(data);
            break;
        }
        case kRtmpMsgTypeBytesRead:
        {
            RTMP_DEBUG << "message bytes read recv.";
            break;
        }        
        case kRtmpMsgTypeUserControl:
        {
            HandleUserMessage(data);
            break;
        }
        case kRtmpMsgTypeWindowACKSize:
        {
            HandleAckWindowSize(data);
            break;
        }
        case kRtmpMsgTypeAMF3Message:
        {
            HandleAmfCommand(data,true);
            break;
        }
        case kRtmpMsgTypeAMFMessage:
        {
            HandleAmfCommand(data);
            break;
        }
        case kRtmpMsgTypeAMFMeta:
        case kRtmpMsgTypeAMF3Meta:
        case kRtmpMsgTypeAudio:
        case kRtmpMsgTypeVideo:
        {
            SetPacketType(data);
            if(rtmp_handler_)
            {
                rtmp_handler_->OnRecv(connection_,std::move(data));
            }
            break;
        }      
        default:
        RTMP_ERROR << " not surpport message type:" << type;
        break;
    }
}
bool RtmpContext::BuildChunk(const PacketPtr &packet,uint32_t timestamp,bool fmt0)
{
    RtmpMsgHeaderPtr h = packet->Ext<RtmpMsgHeader>();
    if(h)
    {
        out_sending_packets_.emplace_back(packet);
        RtmpMsgHeaderPtr &prev = out_message_headers_[h->cs_id];
        bool use_delta = !fmt0 && !prev && timestamp >= prev->timestamp && h->msg_sid == prev->msg_sid;
        if(!prev)
        {
            prev = std::make_shared<RtmpMsgHeader>();
        }
        int fmt = kRtmpFmt0;
        if(use_delta)
        {
            fmt = kRtmpFmt1 ;
            timestamp -= prev->timestamp;
            if(h->msg_type == prev->msg_type
                && h->msg_len == prev->msg_len)
            {
                fmt = kRtmpFmt2;
                if(timestamp == out_deltas_[h->cs_id]) 
                {
                    fmt = kRtmpFmt3;
                }   
            }
        }

        char *p = out_current_;

        if(h->cs_id<64)
        {
            *p++ = (char)((fmt<<6)|h->cs_id);
        }
        else if(h->cs_id<(64+256))
        {
           *p++ = (char)((fmt<<6)|0); 
           *p++ = (char)(h->cs_id - 64);
        }
        else 
        {
           *p++ = (char)((fmt<<6)|1); 
           uint16_t cs = h->cs_id-64;
           memcpy(p,&cs,sizeof(uint16_t));
           p += sizeof(uint16_t);
        }

        auto ts = timestamp;
        if(timestamp >= 0xFFFFFF)
        {
            ts = 0xFFFFFF;
        }

        if(fmt == kRtmpFmt0)
        {
            p += BytesWriter::WriteUint24T(p,ts);
            p += BytesWriter::WriteUint24T(p,h->msg_len);
            p += BytesWriter::WriteUint8T(p,h->msg_type);

            memcpy(p,&h->msg_sid,4);
            p += 4;
            out_deltas_[h->cs_id] = 0;
        } 
        else if(fmt == kRtmpFmt1)
        {
            p += BytesWriter::WriteUint24T(p,ts);
            p += BytesWriter::WriteUint24T(p,h->msg_len);
            p += BytesWriter::WriteUint8T(p,h->msg_type);
            out_deltas_[h->cs_id] = timestamp;
        }
        else if(fmt == kRtmpFmt2)
        {
            p += BytesWriter::WriteUint24T(p,ts);
            out_deltas_[h->cs_id] = timestamp;
        }    

        if(ts == 0xFFFFFF)
        {
            memcpy(p,&timestamp,4);
            p += 4;
        }    

        BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_,p-out_current_);
        sending_bufs_.emplace_back(std::move(nheader));
        out_current_ = p;

        prev->cs_id = h->cs_id;
        prev->msg_len = h->msg_len;
        prev->msg_sid = h->msg_sid;
        prev->msg_type = h->msg_type;
        if(fmt == kRtmpFmt0)
        {
            prev->timestamp = timestamp;
        }
        else 
        {
            prev->timestamp += timestamp;
        }
        
        const char *body = packet->Data();
        int32_t bytes_parsed = 0;
        while(true)
        {
            const char * chunk = body+bytes_parsed;
            int32_t size = h->msg_len - bytes_parsed;
            size = std::min(size,out_chunk_size_);

            BufferNodePtr node = std::make_shared<BufferNode>((void*)chunk,size);
            sending_bufs_.emplace_back(std::move(node));
            bytes_parsed += size;

            if(bytes_parsed<h->msg_len)
            {
                if(out_current_ - out_buffer_ >= 4096)
                {
                    RTMP_ERROR << "rtmp had no enough out header buffer.";
                    break;
                }
                char *p = out_current_;

                if(h->cs_id<64)
                {
                    *p++ = (char)(0xC0|h->cs_id);
                }
                else if(h->cs_id<(64+256))
                {
                    *p++ = (char)(0xC0|0); 
                    *p++ = (char)(h->cs_id - 64);
                }
                else 
                {
                    *p++ = (char)(0xC0|1); 
                    uint16_t cs = h->cs_id-64;
                    memcpy(p,&cs,sizeof(uint16_t));
                    p += sizeof(uint16_t);
                }
                if(ts == 0xFFFFFF)
                {
                    memcpy(p,&timestamp,4);
                    p += 4;
                }      

                BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_,p-out_current_);
                sending_bufs_.emplace_back(std::move(nheader));
                out_current_ = p;      
            }
            else 
            {
                break;
            }
        }
        return true;
    }
    return false;
}
void RtmpContext::Send()
{
    if(sending_)
    {
        return;
    }
    sending_ = true;
    for(int i=0;i<10;i++)
    {
        if(out_waiting_queue_.empty())
        {
            break;
        }
        PacketPtr packet = std::move(out_waiting_queue_.front());
        out_waiting_queue_.pop_front();
        BuildChunk(std::move(packet));
    }
    connection_->Send(sending_bufs_);
}
bool RtmpContext::Ready() const
{
    return !sending_;
}
bool RtmpContext::BuildChunk (PacketPtr &&packet,uint32_t timestamp,bool fmt0)
{
    RtmpMsgHeaderPtr h = packet->Ext<RtmpMsgHeader>();
    if(h)
    {
        RtmpMsgHeaderPtr &prev = out_message_headers_[h->cs_id];
        bool use_delta = !fmt0 && prev && timestamp >= prev->timestamp && h->msg_sid == prev->msg_sid;
        if(!prev)
        {
            prev = std::make_shared<RtmpMsgHeader>();
        }
        int fmt = kRtmpFmt0;
        if(use_delta)
        {
            fmt = kRtmpFmt1 ;
            timestamp -= prev->timestamp;
            if(h->msg_type == prev->msg_type
                && h->msg_len == prev->msg_len)
            {
                fmt = kRtmpFmt2;
                if(timestamp == out_deltas_[h->cs_id]) 
                {
                    fmt = kRtmpFmt3;
                }   
            }
        }

        char *p = out_current_;

        if(h->cs_id<64)
        {
            *p++ = (char)((fmt<<6)|h->cs_id);
        }
        else if(h->cs_id<(64+256))
        {
           *p++ = (char)((fmt<<6)|0); 
           *p++ = (char)(h->cs_id - 64);
        }
        else 
        {
           *p++ = (char)((fmt<<6)|1); 
           uint16_t cs = h->cs_id-64;
           memcpy(p,&cs,sizeof(uint16_t));
           p += sizeof(uint16_t);
        }

        auto ts = timestamp;
        if(timestamp >= 0xFFFFFF)
        {
            ts = 0xFFFFFF;
        }

        if(fmt == kRtmpFmt0)
        {
            p += BytesWriter::WriteUint24T(p,ts);
            p += BytesWriter::WriteUint24T(p,h->msg_len);
            p += BytesWriter::WriteUint8T(p,h->msg_type);

            memcpy(p,&h->msg_sid,4);
            p += 4;
            out_deltas_[h->cs_id] = 0;
        } 
        else if(fmt == kRtmpFmt1)
        {
            p += BytesWriter::WriteUint24T(p,ts);
            p += BytesWriter::WriteUint24T(p,h->msg_len);
            p += BytesWriter::WriteUint8T(p,h->msg_type);
            out_deltas_[h->cs_id] = timestamp;
        }
        else if(fmt == kRtmpFmt2)
        {
            p += BytesWriter::WriteUint24T(p,ts);
            out_deltas_[h->cs_id] = timestamp;
        }    

        if(ts == 0xFFFFFF)
        {
            memcpy(p,&timestamp,4);
            p += 4;
        }    

        BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_,p-out_current_);
        sending_bufs_.emplace_back(std::move(nheader));
        out_current_ = p;

        prev->cs_id = h->cs_id;
        prev->msg_len = h->msg_len;
        prev->msg_sid = h->msg_sid;
        prev->msg_type = h->msg_type;
        if(fmt == kRtmpFmt0)
        {
            prev->timestamp = timestamp;
        }
        else 
        {
            prev->timestamp += timestamp;
        }
        
        const char *body = packet->Data();
        int32_t bytes_parsed = 0;
        while(true)
        {
            const char * chunk = body+bytes_parsed;
            int32_t size = h->msg_len - bytes_parsed;
            size = std::min(size,out_chunk_size_);

            BufferNodePtr node = std::make_shared<BufferNode>((void*)chunk,size);
            sending_bufs_.emplace_back(std::move(node));
            bytes_parsed += size;

            if(bytes_parsed<h->msg_len)
            {
                if(out_current_ - out_buffer_ >= 4096)
                {
                    RTMP_ERROR << "rtmp had no enough out header buffer.";
                    break;
                }
                char *p = out_current_;

                if(h->cs_id<64)
                {
                    *p++ = (char)(0xC0|h->cs_id);
                }
                else if(h->cs_id<(64+256))
                {
                    *p++ = (char)(0xC0|0); 
                    *p++ = (char)(h->cs_id - 64);
                }
                else 
                {
                    *p++ = (char)(0xC0|1); 
                    uint16_t cs = h->cs_id-64;
                    memcpy(p,&cs,sizeof(uint16_t));
                    p += sizeof(uint16_t);
                }
                if(ts == 0xFFFFFF)
                {
                    memcpy(p,&timestamp,4);
                    p += 4;
                }      

                BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_,p-out_current_);
                sending_bufs_.emplace_back(std::move(nheader));
                out_current_ = p;      
            }
            else 
            {
                break;
            }
        }
        out_sending_packets_.emplace_back(std::move(packet));
        return true;
    }
    return false;
}
void RtmpContext::CheckAndSend()
{
    sending_ = false;
    out_current_ = out_buffer_;
    sending_bufs_.clear();
    out_sending_packets_.clear();

    if(!out_waiting_queue_.empty())
    {
        Send();
    }
    else
    {
        if(rtmp_handler_)
        {
            rtmp_handler_->OnActive(connection_);
        }
    }
}
void RtmpContext::PushOutQueue(PacketPtr && packet)
{
    out_waiting_queue_.emplace_back(std::move(packet));
    Send();
}

void RtmpContext::SendSetChunkSize()
{
    PacketPtr packet = Packet::NewPacket(64);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    if(header)
    {
        header->cs_id = kRtmpCSIDCommand;
        header->msg_len = 0;
        header->msg_type = kRtmpMsgTypeChunkSize;
        header->timestamp = 0;
        header->msg_sid = kRtmpMsID0;
        packet->SetExt(header);
    }

    char *body = packet->Data();
    header->msg_len = BytesWriter::WriteUint32T(body,out_chunk_size_);
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "send chuck size:" << out_chunk_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}
void RtmpContext::SendAckWindowSize()
{
    PacketPtr packet = Packet::NewPacket(64);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    if(header)
    {
        header->cs_id = kRtmpCSIDCommand;
        header->msg_len = 0;
        header->msg_type = kRtmpMsgTypeWindowACKSize;
        header->timestamp = 0;
        header->msg_sid = kRtmpMsID0;
        packet->SetExt(header);
    }

    char *body = packet->Data();
    header->msg_len = BytesWriter::WriteUint32T(body,ack_size_);
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "send act size:" << ack_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}
void RtmpContext::SendSetPeerBandwidth()
{
    PacketPtr packet = Packet::NewPacket(64);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    if(header)
    {
        header->cs_id = kRtmpCSIDCommand;
        header->msg_len = 0;
        header->msg_type = kRtmpMsgTypeSetPeerBW;
        header->timestamp = 0;
        header->msg_sid = kRtmpMsID0;
        packet->SetExt(header);
    }

    char *body = packet->Data();

    body += BytesWriter::WriteUint32T(body,ack_size_);
    *body++ = 0x02;
    header->msg_len = 5;
    packet->SetPacketSize(5);
    RTMP_DEBUG << "send band width:" << ack_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}
void RtmpContext::SendBytesRecv()
{
    if(in_bytes_>= ack_size_)
    {
        PacketPtr packet = Packet::NewPacket(64);
        RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
        if(header)
        {
            header->cs_id = kRtmpCSIDCommand;
            header->msg_len = 0;
            header->msg_type = kRtmpMsgTypeBytesRead;
            header->timestamp = 0;
            header->msg_sid = kRtmpMsID0;
            packet->SetExt(header);
        }

        char *body = packet->Data();
        header->msg_len = BytesWriter::WriteUint32T(body,in_bytes_);
        packet->SetPacketSize(header->msg_len);
        //RTMP_DEBUG << "send act size:" << ack_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
        PushOutQueue(std::move(packet));
        in_bytes_ = 0;
    }
}
void RtmpContext::SendUserCtrlMessage(short nType, uint32_t value1, uint32_t value2)
{
    PacketPtr packet = Packet::NewPacket(64);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    if(header)
    {
        header->cs_id = kRtmpCSIDCommand;
        header->msg_len = 0;
        header->msg_type = kRtmpMsgTypeUserControl;
        header->timestamp = 0;
        header->msg_sid = kRtmpMsID0;
        packet->SetExt(header);
    }

    char *body = packet->Data();
    char *p = body;

    p += BytesWriter::WriteUint16T(body,nType);
    p += BytesWriter::WriteUint32T(body,value1);
    if(nType == kRtmpEventTypeSetBufferLength)
    {
        p += BytesWriter::WriteUint32T(body,value2);
    }
    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "send user control type:" << nType 
                << " value:" << value1 
                << " value2:" << value2 
                << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}

void RtmpContext::HandleChunkSize(PacketPtr &packet)
{
    if(packet->PacketSize() >=4)
    {
        auto size = BytesReader::ReadUint32T(packet->Data());
        RTMP_DEBUG << "recv chunk size in_chunk_size:" << in_chunk_size_ << " change to " << size;
        in_chunk_size_ = size;
    }
    else 
    {
        RTMP_ERROR << "invalid chunk size packet msg_len:" << packet->PacketSize()
        << " host:" << connection_->PeerAddr().ToIpPort();
    }
}
void RtmpContext::HandleAckWindowSize(PacketPtr &packet)
{
    if(packet->PacketSize() >=4)
    {
        auto size = BytesReader::ReadUint32T(packet->Data());
        RTMP_DEBUG << "recv ack window size ack_size_:" << ack_size_ << " change to " << size;
        ack_size_ = size;
    }
    else 
    {
        RTMP_ERROR << "invalid ack window size packet msg_len:" << packet->PacketSize()
            << " host:" << connection_->PeerAddr().ToIpPort();
    }    
}
void RtmpContext::HandleUserMessage(PacketPtr &packet)
{
    auto msg_len = packet->PacketSize();
    if(msg_len < 6)
    {
        RTMP_ERROR << "invalid user control packet msg_len:" << packet->PacketSize()
        << " host:" << connection_->PeerAddr().ToIpPort();
        return;
    }
    char * body = packet->Data();
    auto type = BytesReader::ReadUint16T(body);
    auto value = BytesReader::ReadUint32T(body+2);

    RTMP_DEBUG << "recv user control type:"<<type<<" value" << value << " host:" << connection_->PeerAddr().ToIpPort();
    switch(type)
    {
        case kRtmpEventTypeStreamBegin:
        {
            RTMP_DEBUG << "recv stream begin value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            break;
        }
        case kRtmpEventTypeStreamEOF:
        {
            RTMP_DEBUG << "recv stream eof value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            break;
        }   
        case kRtmpEventTypeStreamDry:
        {
            RTMP_DEBUG << "recv stream dry value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            break;
        }
        case kRtmpEventTypeSetBufferLength:
        {
            RTMP_DEBUG << "recv set buffer length value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            if(msg_len<10)
            {
                RTMP_ERROR << "invalid user control packet msg_len:" << packet->PacketSize()
                << " host:" << connection_->PeerAddr().ToIpPort();                
                return ;
            }
            break;
        }   
        case kRtmpEventTypeStreamsRecorded:
        {
            RTMP_DEBUG << "recv stream recoded value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            break;
        }
        case kRtmpEventTypePingRequest:
        {
            RTMP_DEBUG << "recv ping request value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            SendUserCtrlMessage(kRtmpEventTypePingResponse,value,0);
            break;
        }   
        case kRtmpEventTypePingResponse:
        {
            RTMP_DEBUG << "recv ping response value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            break;
        }
        default:
            break;                             
    }
}

void RtmpContext::HandleAmfCommand(PacketPtr &data,bool amf3)
{
    RTMP_DEBUG << "amf message len:" << data->PacketSize() << " host:" << connection_->PeerAddr().ToIpPort();

    const char *body = data->Data();
    int32_t msg_len = data->PacketSize();

    if(amf3)
    {
        body += 1;
        msg_len -= 1;
    }

    AMFObject obj;
    if(obj.Decode(body,msg_len)<0)
    {
        RTMP_ERROR << "amf decode failed. host:" << connection_->PeerAddr().ToIpPort();
        return;
    }
    const std::string &method = obj.Property(0)->String();
    RTMP_DEBUG << "amf command:" << method << " host:" << connection_->PeerAddr().ToIpPort();
    auto iter = commands_.find(method);
    if(iter == commands_.end())
    {
        RTMP_DEBUG << "not surpport method:" << method << " host:" << connection_->PeerAddr().ToIpPort();
        return ;
    }
    iter->second(obj);
}

void RtmpContext::SendConnect()
{
    SendSetChunkSize();
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;
    header->msg_sid = 0;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; 
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "connect");
    p += AMFAny::EncodeNumber(p, 1.0);
    *p++ = kAMFObject;
    p += AMFAny::EncodeNamedString(p, "app", app_);
    p += AMFAny::EncodeNamedString(p, "tcUrl", tc_url_);
    p += AMFAny::EncodeNamedBoolean(p, "fpad", false);
    p += AMFAny::EncodeNamedNumber(p, "capabilities", 31.0);
    p += AMFAny::EncodeNamedNumber(p, "audioCodecs", 1639.0);
    p += AMFAny::EncodeNamedNumber(p, "videoCodecs", 252.0);
    p += AMFAny::EncodeNamedNumber(p, "videoFunction", 1.0);
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;

    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "send connect msg_len:" << header->msg_len << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}
void RtmpContext::HandleConnect(AMFObject &obj)
{
    auto amf3 = false;
    tc_url_ = obj.Property("tcUrl")->String();
    AMFObjectPtr sub_obj = obj.Property(2)->Object();
    if(sub_obj)
    {
        app_ = sub_obj->Property("app")->String();
        if(sub_obj->Property("objectEncoding"))
        {
            amf3 = sub_obj->Property("objectEncoding")->Number() == 3.0;
        }
    }

    RTMP_DEBUG << "recv connect tcUrl:" << tc_url_ << " app:" << app_ << " amf3:" << amf3;
    SendAckWindowSize();
    SendSetPeerBandwidth();
    SendSetChunkSize();
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;
    header->msg_sid = 0;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; 
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "_result");
    p += AMFAny::EncodeNumber(p, 1.0);
    *p++ = kAMFObject;
    p += AMFAny::EncodeNamedString(p, "fmsVer", "FMS/3,0,1,123");
    p += AMFAny::EncodeNamedNumber(p, "capabilities", 31);
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;
    *p++ = kAMFObject; 
    p += AMFAny::EncodeNamedString(p, "level", "status");
    p += AMFAny::EncodeNamedString(p, "code", "NetConnection.Connect.Success");
    p += AMFAny::EncodeNamedString(p, "description", "Connection succeeded.");
    p += AMFAny::EncodeNamedNumber(p, "objectEncoding", amf3 ? 3.0 : 0);
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;

    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "connect result msg_len:" << header->msg_len << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}

void RtmpContext::SendCreateStream()
{
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;
    header->msg_sid = 0;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; 
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "createStream");
    p += AMFAny::EncodeNumber(p, 4.0);
    *p++ = kAMFNull;

    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "send create stream msg_len:" << header->msg_len << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}
void RtmpContext::HandleCreateStream(AMFObject &obj)
{
    auto tran_id = obj.Property(1)->Number();

    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;
    header->msg_sid = 0;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; 
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "_result");
    p += AMFAny::EncodeNumber(p, tran_id);
    *p++ = kAMFNull;
    
    p += AMFAny::EncodeNumber(p, kRtmpMsID1);

    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "create stream result msg_len:" << header->msg_len << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}
void RtmpContext::SendStatus(const std::string &level, const std::string &code, const std::string &description)
{
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;
    header->msg_sid = 1;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; 
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "onStatus");
    p += AMFAny::EncodeNumber(p, 0);
    *p++ = kAMFNull;
    *p++ = kAMFObject;
    p += AMFAny::EncodeNamedString(p, "level", level);
    p += AMFAny::EncodeNamedString(p, "code", code);
    p += AMFAny::EncodeNamedString(p, "description", description);
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;


    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "send status level:" << level << " code:" << code << " desc:" << description << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}

void RtmpContext::SendPlay()
{
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;
    header->msg_sid = 1;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; 
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "play");
    p += AMFAny::EncodeNumber(p, 0);
    *p++ = kAMFNull;
    p += AMFAny::EncodeString(p, name_);
    p += AMFAny::EncodeNumber(p, -1000.0);

    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "send play name:"<< name_ 
            << " msg_len:" << header->msg_len 
            << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}
void RtmpContext::HandlePlay(AMFObject &obj)
{
    auto tran_id = obj.Property(1)->Number();
    name_ = obj.Property(3)->String();
    ParseNameAndTcUrl();

    RTMP_DEBUG << "recv play session_name:" << session_name_ 
            << " param:" << param_ 
            << " host:" << connection_->PeerAddr().ToIpPort();
    is_player_ = true;
    SendUserCtrlMessage(kRtmpEventTypeStreamBegin,1,0);
    SendStatus("status","NetStream.Play.Start","Start Playing");

    if(rtmp_handler_)
    {
        rtmp_handler_->OnPlay(connection_,session_name_,param_);
    }
}
void RtmpContext::ParseNameAndTcUrl()
{
    auto pos = app_.find_first_of("/");
    if(pos!=std::string::npos)
    {
        app_ = app_.substr(pos+1);
    }

    param_.clear();
    pos = name_.find_first_of("?");
    if(pos != std::string::npos)
    {
        param_ = name_.substr(pos+1);
        name_ = name_.substr(0,pos);
    }

    std::string domain;

    std::vector<std::string> list = base::StringUtils::SplitString(tc_url_,"/");
    if(list.size() == 6)
    {
        domain = list[3];
        app_ = list[4];
        name_ = list[5];
    } 
    else if(list.size()==5)//rmtp://ip/domain:port/app
    {
        domain = list[3];
        app_ = list[4];
    }
    else if(list.size() == 4) //rmtp://domain:port/app
    {
        domain = list[2];
        app_ = list[3];
    }

    auto p = domain.find_first_of(":");
    if(p!=std::string::npos)
    {
        domain = domain.substr(0,p);
    }

    session_name_.clear();
    session_name_ += domain;
    session_name_ += "/";
    session_name_ += app_;
    session_name_ += "/";
    session_name_ += name_;

    RTMP_DEBUG << "session_name:" << session_name_ 
            << " param:" << param_ 
            << " host:" << connection_->PeerAddr().ToIpPort();
}

void RtmpContext::SendPublish()
{
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;
    header->msg_sid = 1;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; 
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "publish");
    p += AMFAny::EncodeNumber(p, 5);
    *p++ = kAMFNull;
    p += AMFAny::EncodeString(p, name_);
    p += AMFAny::EncodeString(p, "live");


    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_DEBUG << "send publish name:"<< name_ 
            << " msg_len:" << header->msg_len 
            << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}
void RtmpContext::HandlePublish(AMFObject &obj)
{
    auto tran_id = obj.Property(1)->Number();
    name_ = obj.Property(3)->String();
    ParseNameAndTcUrl();

    RTMP_DEBUG << "recv publish session_name:" << session_name_ 
            << " param:" << param_ 
            << " host:" << connection_->PeerAddr().ToIpPort();
    is_player_ = false;
    SendStatus("status","NetStream.Publish.Start","Start Publishing");

    if(rtmp_handler_)
    {
        rtmp_handler_->OnPublish(connection_,session_name_,param_);
    }

}

void RtmpContext::HandleResult(AMFObject &obj)
{
    auto id = obj.Property(1)->Number();
    RTMP_DEBUG << "recv result id:" << id << " host:" << connection_->PeerAddr().ToIpPort();
    if(id == 1)
    {
        SendCreateStream();
    }
    else if(id == 4)
    {
        if(is_player_)
        {
            SendPlay();
        }
        else 
        {
            SendPublish();
        }
    }
    else if(id == 5)
    {
        if(rtmp_handler_)
        {
            rtmp_handler_->OnPublishPrepare(connection_);
        }
    }

}
void RtmpContext::HandleError(AMFObject &obj)
{
    const std::string &description = obj.Property(3)->Object()->Property("description")->String();
    RTMP_ERROR << "recv error description:" << description << " host:" << connection_->PeerAddr().ToIpPort();
    connection_->ForceClose();
}

void RtmpContext::HandleStatus(AMFObject &obj)
{
    const std::string &code  = obj.Property(3)->Object()->Property("code")->String();
    const std::string &level = obj.Property(3)->Object()->Property("level")->String();
    LOG_INFO << "recv status:" << code << ", level:" << level << ", ip:" << connection_->PeerAddr().ToIpPort();
    if (code == "NetStream.Publish.Start") 
    {
        rtmp_handler_->OnPublish(connection_, session_name_, param_);
    } 
    else if (code == "NetStream.Play.Start") 
    {
        rtmp_handler_->OnPlay(connection_, session_name_, param_);
    } 
    else if (code == "NetStream.Play.StreamNotFound") 
    {
    } 
    else if (code == "NetStream.Play.Failed") 
    {
    }
}

void RtmpContext::Play(const std::string &url)
{
    is_client_ = true;
    is_player_ = true;
    tc_url_ = url;
    ParseNameAndTcUrl();
}
void RtmpContext::Publish(const std::string &url)
{
    is_client_ = true;
    is_player_ = false;
    tc_url_ = url;
    ParseNameAndTcUrl();
}