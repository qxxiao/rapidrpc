#ifndef RAPIDRPC_NET_TCP_TCP_SERVER_H
#define RAPIDRPC_NET_TCP_TCP_SERVER_H

#include "net/tcp/tcp_acceptor.h"
#include "net/eventloop.h"
#include "net/io_thread_group.h"
#include "net/tcp/tcp_connection.h"
#include <set>

namespace rapidrpc {

class TcpServer {
public:
    /**
     * 根据本地地址创建一个 TcpServer 对象，用于监听连接
     * @param local_addr 本地监听地址
     * @note 使用地址的协议族和端口号，SOCK_STREAM 套接字类型
     */
    TcpServer(NetAddr::s_ptr local_addr);

    ~TcpServer();

    /**
     * @brief 启动 TcpServer 的 MainReactor 的 EventLoop 以及 SubReactor 的 EventLoop
     */
    void start();

    // 删除连接，由 TcpConnection 调用
    void removeConnection(TcpConnection::w_ptr conn);

private:
    // 初始化 EventFd, 将 Acceptor 的 fd 添加到主Reactor的监听集合中
    void init();
    // Acceptor 的回调函数，用于处理新连接
    void onAccept();

private:
    TcpAcceptor::s_ptr m_acceptor;
    NetAddr::s_ptr m_local_addr; // 本地监听地址

    EventLoop *m_main_event_loop{nullptr};     // mainReactor 用于创建连接
    IOThreadGroup *m_io_thread_group{nullptr}; // subReactor 用于处理连接

    // 每次添加 FdEvent*, 如果在添加到 SubReactor
    // 时，需要到回调函数中才执行添加动作，如果是局部变量，会在函数结束时析构，导致无法添加到 SubReactor
    // 全局变量
    FdEvent *m_listen_fd_event{nullptr}; // 监听 fd 的事件

    int m_client_counts{0}; // 客户端连接数

    // 全局变量，用于保存所有的连接
    std::set<TcpConnection::s_ptr> m_connections;
    std::mutex m_mutex; // 保护 m_connections
};
} // namespace rapidrpc

#endif // RAPIDRPC_NET_TCP_TCP_SERVER_H