#ifndef RAPIDRPC_NET_TCP_TCP_CONNECTION_H
#define RAPIDRPC_NET_TCP_TCP_CONNECTION_H

#include "net/tcp/net_addr.h"
#include "net/tcp/tcp_buffer.h"
#include "net/io_thread.h"

namespace rapidrpc {

enum class TcpState { NotConnected = 1, Connected = 2, HalfClosed = 3, Closed = 4 };

class TcpConnection {
public:
    using s_ptr = std::shared_ptr<TcpConnection>;
    using w_ptr = std::weak_ptr<TcpConnection>;

public:
    TcpConnection(IOThread *io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr);

    ~TcpConnection();

    // read data from socket, and parse it
    void onRead();

    // execute the request
    void execute();

    // write the response to socket
    void onWrite();

    void setState(const TcpState state);
    TcpState getState() const;

    // 主动关闭无效的恶意连接
    // shutdwon the connection(unvalid tcp connection)
    void shutdown();

    // 设置连接关闭时的回调函数，绑定到 TcpServer::removeConnection，在创建连接后设置
    void setRemoveConnCb(std::function<void()> &&remove_conn_cb);

private:
    NetAddr::s_ptr m_local_addr;
    NetAddr::s_ptr m_peer_addr;

    // InBuffer: socket 通过 read 将 socket 缓冲区数据读入到 InBuffer
    // 服务端通过读取 InBuffer 中的数据，解析出请求
    TcpBuffer::s_ptr m_in_buffer;

    // OutBuffer: 服务端将响应的数据编码后写入到 OutBuffer
    // socket 通过 write 将 OutBuffer 中的数据写入到 socket 缓冲区发送给客户端
    TcpBuffer::s_ptr m_out_buffer;

    IOThread *m_io_thread;        // 当前连接所在的 IOThread线程
    FdEvent *m_fd_event{nullptr}; // 当前连接的fd事件

    TcpState m_state; // 当前连接的状态

    std::function<void()> m_remove_conn_cb; // 删除连接的回调函数
};

} // namespace rapidrpc

#endif // RAPIDRPC_NET_TCP_TCP_CONNECTION_H