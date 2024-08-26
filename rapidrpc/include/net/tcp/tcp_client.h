#ifndef RAPIDRPC_NET_TCP_TCP_CLIENT_H
#define RAPIDRPC_NET_TCP_TCP_CLIENT_H

#include "net/tcp/net_addr.h"
#include "net/eventloop.h"
#include "net/tcp/tcp_connection.h"
#include "net/coder/abstract_protocol.h"
#include "net/timer_event.h"

namespace rapidrpc {

class TcpClient {
public:
    using s_ptr = std::shared_ptr<TcpClient>;

public:
    TcpClient(NetAddr::s_ptr peer_addr);
    ~TcpClient();

    // 异步的连接远程地址 connect to peer_addr
    // 无论连接成功与否，会调用回调函数 conn_cb，上层根据错误码判断是否连接成功
    void connect(std::function<void()> conn_cb);

    // 异步的发送数据
    void writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> write_cb);

    // 异步的读取数据
    void readMessage(const std::string &msg_id, std::function<void(AbstractProtocol::s_ptr)> read_cb);

    // 关闭 EventLoop,关闭所有连接
    void close();

    // check if connected
    int getConnectErrorCode() const;
    std::string getConnectErrorInfo() const;

    NetAddr::s_ptr getPeerAddr() const;
    NetAddr::s_ptr getLocalAddr() const;
    void initLocalAddr();

    // rpc timer
    void addTimerEvent(TimerEvent::s_ptr timer_event);
    void deleteTimerEvent(TimerEvent::s_ptr timer_event);

private:
    NetAddr::s_ptr m_peer_addr;       // 远程地址
    NetAddr::s_ptr m_local_addr;      // 本地地址
    EventLoop *m_event_loop{nullptr}; // 用于处理连接的 EventLoop

    int m_fd{-1};                 // 连接的 socket fd
    FdEvent *m_fd_event{nullptr}; // 连接的事件

    TcpConnection::s_ptr m_connection; // 连接对象

    int m_connect_error_code{0};      // 连接错误码
    std::string m_connect_error_info; // 连接错误信息
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_TCP_TCP_CLIENT_H