#include "TcpConnection.h"  
#include "network/base/Network.h"  
#include <unistd.h>  // 引入unistd库，提供系统调用功能，如 close()  
#include <iostream>  // 用于打印信息  

using namespace tmms::network;  // 使用 tmms::network 命名空间，避免写长路径名  

// 构造函数：初始化 TcpConnection 对象  
TcpConnection::TcpConnection(EventLoop *loop, 
            int socketfd, 
            const InetAddress &localAddr, 
            const InetAddress &peerAddr)
:Connection(loop,socketfd,localAddr,peerAddr)            
{
    // 初始化基类 Connection，传入事件循环、socket 文件描述符、本地地址、对端地址  
    // 相当于新建一个 TCP 连接的管理对象，但此处未做其他初始化逻辑  
}

// 析构函数：当对象销毁时执行  
TcpConnection::~TcpConnection()
{
    loop_->RunInLoop([this](){
        OnClose();  // 在事件循环中调用关闭连接的函数
    });
    NETWORK_DEBUG << "TcpConnection:" << peer_addr_.ToIpPort() <<" destroy.";  
    // 打印调试信息，标识连接被销毁，显示对端地址
}

// 设置关闭回调函数（拷贝版本）  
void TcpConnection::SetCloseCallback(const CloseConnectionCallback &cb)
{
    close_cb_ = cb;  // 将传入的回调函数保存起来，用于触发关闭事件
}

// 设置关闭回调函数（移动版本）  
void TcpConnection::SetCloseCallback(CloseConnectionCallback &&cb)
{
    close_cb_ = std::move(cb);  // 移动回调函数，避免拷贝，提升效率  
}

// 处理关闭事件  
void TcpConnection::OnClose()
{
    loop_->AssertInLoopThread();  // 确保当前操作在事件循环线程中执行  
    if(!closed_)  // 判断是否已经关闭过，避免重复关闭  
    {
        closed_ = true;  // 标记连接已关闭  
        if(close_cb_)  // 如果设置了关闭回调函数，则执行回调  
        {
            close_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
        }
        Event::Close();  // 执行基类 Event 的关闭逻辑
    }
}

// 强制关闭连接  
void TcpConnection::ForceClose()
{
    loop_->RunInLoop([this](){
        OnClose();  // 在事件循环中触发关闭事件  
    });
}

// 设置接收消息回调函数（拷贝版本）  
void TcpConnection::SetRecvMsgCallback(const MessageCallback &cb)
{
    message_cb_ = cb;  // 保存接收消息的回调函数  
}

// 设置接收消息回调函数（移动版本）  
void TcpConnection::SetRecvMsgCallback(MessageCallback &&cb)
{
    message_cb_ = std::move(cb);  // 移动回调函数，避免拷贝  
}

// 读取数据的回调函数  
void TcpConnection::OnRead()
{
    if(closed_)  // 如果连接已关闭，直接返回  
    {
        NETWORK_TRACE << "host:" << peer_addr_.ToIpPort() << " had closed.";
        return;
    }
    ExtendLife();  // 扩展连接的生命周期（防止超时断开）  
    while (true)  // 循环读取数据  
    {
        int err = 0;
        auto ret = message_buffer_.ReadFd(fd_,&err);  // 从 socket 读取数据到缓冲区  
        if(ret > 0)  // 成功读取到数据  
        {
            if(message_cb_)  // 如果有消息回调函数，调用回调处理数据  
            {
                message_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()),message_buffer_);
            }
        }
        else if( ret == 0)  // 如果读取返回 0，表示对端关闭连接  
        {
            OnClose();  // 触发关闭逻辑  
            break;
        }
        else 
        {
            if(err != EINTR && err != EAGAIN && err != EWOULDBLOCK)  // 处理读取错误  
            {
                OnClose();  // 关闭连接  
            }
            break;
        }
    }
}

// 处理错误的回调函数  
void TcpConnection::OnError(const std::string &msg)
{
    NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << " error msg:" << msg;  
    OnClose();  // 发生错误时，触发关闭逻辑  
}

// 设置写入完成回调函数（拷贝版本）  
void TcpConnection::SetWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    write_complete_cb_ = cb;
}

// 设置写入完成回调函数（移动版本）  
void TcpConnection::SetWriteCompleteCallback(WriteCompleteCallback &&cb)
{
    write_complete_cb_ = std::move(cb);
}

// 发送数据写入的回调函数  
void TcpConnection::OnWrite()
{
    if(closed_)  // 如果连接已关闭，直接返回  
    {
        NETWORK_TRACE << "host:" << peer_addr_.ToIpPort() << " had closed.";
        return;
    }
    ExtendLife();  // 扩展连接的生命周期  
    if(!io_vec_list_.empty())  // 如果有待写入的数据  
    {
        while(true)
        {
            auto ret = ::writev(fd_,&io_vec_list_[0],io_vec_list_.size());  // 批量写入数据  
            if(ret >= 0)
            {
                while(ret > 0)
                {
                    if(io_vec_list_.front().iov_len > ret)  
                    {
                        io_vec_list_.front().iov_base =(char*)io_vec_list_.front().iov_base+ret;
                        io_vec_list_.front().iov_len -= ret;  
                        break;
                    }
                    else 
                    {
                        ret -= io_vec_list_.front().iov_len;
                        io_vec_list_.erase(io_vec_list_.begin());
                    }
                }
                if(io_vec_list_.empty())  // 如果所有数据写完  
                {
                    EnableWriting(false);  // 停止写入事件  
                    if(write_complete_cb_)  // 触发写入完成回调  
                    {
                        write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
                    }
                    return;
                }
            }
            else 
            {
                if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)  // 发生写入错误  
                {
                    NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << " write err:" << errno;
                    OnClose();  // 触发关闭逻辑  
                    return;
                }
                break;
            }
        }
    }
    else 
    {
        EnableWriting(false);  // 如果没有数据待写，停止写入事件  
        if(write_complete_cb_)  // 触发写入完成回调  
        {
            write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
        }
    }
}

void TcpConnection::Send(std::list<BufferNodePtr>&list)
{
    loop_->RunInLoop([this,&list](){
        SendInLoop(list);
    });
}
void TcpConnection::Send(const char *buf,size_t size)
{
    loop_->RunInLoop([this,buf,size](){
        SendInLoop(buf,size);
    });
}

void TcpConnection::SendInLoop(const char *buf,size_t size)
{
    if(closed_)
    {
        NETWORK_TRACE << "host:" << peer_addr_.ToIpPort() << " had closed.";
        return;
    }
    size_t send_len = 0;
    if(io_vec_list_.empty())
    {
        send_len = ::write(fd_,buf,size);
        if(send_len<0)
        {
            if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << " write err:" << errno;
                OnClose();
                return;
            }
            send_len = 0;
        }
        size -= send_len;
        if(size==0)
        {
            if(write_complete_cb_)
            {
                write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
            }
            return;
        }
    }
    if(size>0)
    {
        struct iovec vec;
        vec.iov_base = (void*)(buf+send_len);
        vec.iov_len = size;
        
        io_vec_list_.push_back(vec);
        EnableWriting(true);
    }
}
void TcpConnection::SendInLoop(std::list<BufferNodePtr>&list)
{
    if(closed_)
    {
        NETWORK_TRACE << "host:" << peer_addr_.ToIpPort() << " had closed.";
        return;
    }
    for(auto &l:list)
    {
        struct iovec vec;
        vec.iov_base = (void*)l->addr;
        vec.iov_len = l->size;
        
        io_vec_list_.push_back(vec);
    }
    if(!io_vec_list_.empty())
    {
        EnableWriting(true);
    }
}
void TcpConnection::SetTimeoutCallback(int timeout,const TimeoutCallback &cb)
{
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());
    loop_->RunAfter(timeout,[&cp,&cb](){
        cb(cp);
    });
}
void TcpConnection::SetTimeoutCallback(int timeout,TimeoutCallback &&cb)
{
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());
    loop_->RunAfter(timeout,[&cp,cb](){
        cb(cp);
    });
}
void TcpConnection::OnTimeout()
{
    NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << " timeout and close it.";
    //std::cout << "host:" << peer_addr_.ToIpPort() << " timeout and close it." << std::endl;
    OnClose();
}
void TcpConnection::EnableCheckIdleTimeout(int32_t max_time)
{
    auto tp = std::make_shared<TimeoutEntry>(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
    max_idle_time_ = max_time;
    timeout_entry_ = tp;
    loop_->InsertEntry(max_time,tp);
}
void TcpConnection::ExtendLife()
{
    auto tp = timeout_entry_.lock();
    if(tp)
    {
        loop_->InsertEntry(max_idle_time_,tp);
    }
}


// TcpConnection::Send：发送数据，支持 std::list<BufferNodePtr> 类型
void TcpConnection::Send(std::list<BufferNodePtr>&list)  
{  
    // 将发送操作通过事件循环执行  
    // [this, &list] 表示捕获当前对象指针和列表引用  
    loop_->RunInLoop([this,&list](){  
        SendInLoop(list);  // 调用内部发送函数，真正执行发送逻辑  
    });  
}  

// TcpConnection::Send：发送数据，支持 const char* 数据缓冲区  
void TcpConnection::Send(const char *buf,size_t size)  
{  
    // 同样将发送操作通过事件循环执行  
    // 捕获参数 buf 和 size，确保数据被传递到事件循环中  
    loop_->RunInLoop([this,buf,size](){  
        SendInLoop(buf,size);  // 调用内部发送函数，真正执行发送逻辑  
    });  
}  

// TcpConnection::SendInLoop：内部发送函数（字符缓冲区版本）  
void TcpConnection::SendInLoop(const char *buf,size_t size)  
{  
    if(closed_) // 检查连接是否已经关闭  
    {  
        NETWORK_TRACE << "host:" << peer_addr_.ToIpPort() << " had closed.";  
        return; // 如果关闭，直接返回  
    }  
    size_t send_len = 0;  // 已发送字节数  

    if(io_vec_list_.empty()) // 如果之前没有未发送的数据  
    {  
        send_len = ::write(fd_,buf,size);  // 尝试发送数据到套接字  
        if(send_len<0) // 检查发送是否出错  
        {  
            if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) // 非可重试错误  
            {  
                NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << " write err:" << errno;  
                OnClose(); // 关闭连接  
                return;  
            }  
            send_len = 0;  // 如果是临时错误，已发送字节数设为 0  
        }  
        size -= send_len; // 更新剩余未发送字节数  
        if(size==0) // 如果数据发送完毕  
        {  
            if(write_complete_cb_) // 如果有写完成回调，执行回调  
            {  
                write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));  
            }  
            return;  
        }  
    }  
    if(size>0) // 如果还有数据未发送  
    {  
        struct iovec vec; // 定义 iovec 结构存储未发送数据  
        vec.iov_base = (void*)(buf+send_len);  // 未发送数据的起始地址  
        vec.iov_len = size;  // 未发送数据的长度  
        io_vec_list_.push_back(vec);  // 将 iovec 添加到发送队列  
        EnableWriting(true); // 启用可写事件，等待下一次发送  
    }  
}  

// TcpConnection::SendInLoop：内部发送函数（列表版本）  
void TcpConnection::SendInLoop(std::list<BufferNodePtr>&list)  
{  
    if(closed_) // 检查连接是否关闭  
    {  
        NETWORK_TRACE << "host:" << peer_addr_.ToIpPort() << " had closed.";  
        return;  
    }  
    for(auto &l:list) // 遍历传入的列表  
    {  
        struct iovec vec; // 定义 iovec 结构  
        vec.iov_base = (void*)l->addr; // 数据起始地址  
        vec.iov_len = l->size; // 数据长度  
        io_vec_list_.push_back(vec); // 将数据存入发送队列  
    }  
    if(!io_vec_list_.empty()) // 如果有未发送数据  
    {  
        EnableWriting(true); // 启用写事件，等待发送  
    }  
}  

// TcpConnection::SetTimeoutCallback：设置超时回调（左值版本）  
void TcpConnection::SetTimeoutCallback(int timeout,const TimeoutCallback &cb)  
{  
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());  
    loop_->RunAfter(timeout,[&cp,&cb](){  
        cb(cp); // 触发回调函数，处理超时事件  
    });  
}  

// TcpConnection::SetTimeoutCallback：设置超时回调（右值版本）  
void TcpConnection::SetTimeoutCallback(int timeout,TimeoutCallback &&cb)  
{  
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());  
    loop_->RunAfter(timeout,[&cp,cb](){  
        cb(cp); // 触发回调函数，处理超时事件  
    });  
}  

// TcpConnection::OnTimeout：处理连接超时事件  
void TcpConnection::OnTimeout()  
{  
    NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << " timeout and close it.";  
    OnClose(); // 调用关闭函数，关闭连接  
}  

// TcpConnection::EnableCheckIdleTimeout：启用空闲超时检查  
void TcpConnection::EnableCheckIdleTimeout(int32_t max_time)  
{  
    auto tp = std::make_shared<TimeoutEntry>(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));  
    max_idle_time_ = max_time;  // 设置最大空闲时间  
    timeout_entry_ = tp; // 保存超时条目  
    loop_->InsertEntry(max_time,tp); // 将超时条目插入事件循环中  
}  

// TcpConnection::ExtendLife：延长连接的生命周期  
void TcpConnection::ExtendLife()  
{  
    auto tp = timeout_entry_.lock(); // 获取超时条目的弱引用  
    if(tp) // 如果超时条目有效  
    {  
        loop_->InsertEntry(max_idle_time_,tp); // 重新插入事件循环，延长生命周期  
    }  
}  
