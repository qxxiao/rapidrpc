#ifndef RAPIDRPC_NET_TCP_TCP_CONNECTION_H
#define RAPIDRPC_NET_TCP_TCP_CONNECTION_H

#include "net/tcp/net_addr.h"
#include "net/tcp/tcp_buffer.h"
#include "net/io_thread.h"
#include "net/coder/abstract_protocol.h"
#include "net/coder/abstract_coder.h"

#include <vector>
#include <map>
#include <string>

namespace rapidrpc {

enum class TcpState { NotConnected = 1, Connected = 2, HalfClosed = 3, Closed = 4 };
enum class TcpConnectionType {
    TcpConnectionByServer = 1, // 服务端使用的连接
    TcpConnectionByClient = 2  // 客户端使用的连接
};

class TcpConnection {
public:
    using s_ptr = std::shared_ptr<TcpConnection>;
    using w_ptr = std::weak_ptr<TcpConnection>;

public:
    // TcpConnection(IOThread *io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr);
    TcpConnection(EventLoop *event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr,
                  TcpConnectionType conn_type = TcpConnectionType::TcpConnectionByServer);

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

    // 监听可写事件，并重新加入到 epoll 中
    void listenWriteEvent();

    // 监听可读事件，并重新加入到 epoll 中
    void listenReadEvent();

    // client conn: 添加数据和回调函数到 m_write_cb 队列中，等待下次可写事件发生进行发送和执行回调函数
    void addMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> write_cb);
    // client conn: 添加请求 id 和回调函数到 m_read_cb 哈希表中，等待下次写读事件发生时读取并执行回调函数
    void addReadCb(const std::string &req_id, std::function<void(AbstractProtocol::s_ptr)> read_cb);

private:
    NetAddr::s_ptr m_local_addr;
    NetAddr::s_ptr m_peer_addr;

    // InBuffer: socket 通过 read 将 socket 缓冲区数据读入到 InBuffer
    // 服务端通过读取 InBuffer 中的数据，解析出请求
    TcpBuffer::s_ptr m_in_buffer;

    // OutBuffer: 服务端将响应的数据编码后写入到 OutBuffer
    // socket 通过 write 将 OutBuffer 中的数据写入到 socket 缓冲区发送给客户端
    TcpBuffer::s_ptr m_out_buffer;

    // IOThread *m_io_thread;        // 当前连接所在的 IOThread线程
    EventLoop *m_event_loop;      // 当前连接所在的 EventLoop
    FdEvent *m_fd_event{nullptr}; // 当前连接的fd事件

    TcpState m_state; // 当前连接的状态

    std::function<void()> m_remove_conn_cb; // 服务器端删除连接的回调函数

    TcpConnectionType m_conn_type; // 连接类型，服务端连接或者客户端连接

    // 客户端写入的原始数据(由 m_coder 编码)和对应回调函数
    std::vector<std::pair<AbstractProtocol::s_ptr, std::function<void(AbstractProtocol::s_ptr)>>> m_write_cb;
    // 客户端读取时的请求 id 和回调函数
    std::map<std::string, std::function<void(AbstractProtocol::s_ptr)>> m_read_cb;

    AbstractCoder::s_ptr m_coder; // 编解码器
};

} // namespace rapidrpc

#endif // RAPIDRPC_NET_TCP_TCP_CONNECTION_H