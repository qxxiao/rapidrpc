#include "common/config.h"
#include "net/eventloop.h"
#include "net/fd_event.h"
#include "common/log.h"
#include "net/timer.h" // dup in eventloop.h

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

using namespace std;
using TriggerEvent = rapidrpc::FdEvent::TriggerEvent;

int main() {
    rapidrpc::Config::SetGlobalConfig("/home/xiao/rapidrpc/rapidrpc/conf/rapidrpc.xml");

    rapidrpc::EventLoop eventloop; // contains a wakeup fd event

    // create a listen socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        ERRORLOG("Failed to create listen socket");
        return -1;
    }

    // bind the listen socket
    sockaddr_in addr; // net address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // inet_aton("127.0.0.1", &addr.sin_addr);

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int rt = bind(listen_fd, (sockaddr *)&addr, sizeof(addr));
    if (rt < 0) {
        ERRORLOG("Failed to bind listen socket");
        return -1;
    }

    rt = listen(listen_fd, 100);
    if (rt < 0) {
        ERRORLOG("Failed to listen on listen_fd");
        return -1;
    }

    //!!! fdEvent 生命周期应该在 eventloop 之内，直到关联的 fd 事件结束
    rapidrpc::FdEvent lfd_event(listen_fd);
    lfd_event.listen(TriggerEvent::IN_EVENT, [listen_fd]() {
        // 可读取事件的回调函数
        sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, sizeof(peer_addr));
        int client_fd = accept(listen_fd, (sockaddr *)&peer_addr, &peer_addr_len);
        if (client_fd < 0) {
            ERRORLOG("Failed to accept client connection");
            return;
        }
        DEBUGLOG("Accepted a client connection [%s:%d]", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
    });
    eventloop.addEpollEvent(&lfd_event);

    // test Timer&TimerEvent
    // 1000ms, repeat, lambda
    int i = 0;
    rapidrpc::TimerEvent::s_ptr timer_event = std::make_shared<rapidrpc::TimerEvent>(1000, true, [&i]() {
        INFOLOG("Timer event triggered, i = %d", i++);
    });
    eventloop.addTimerEvent(timer_event); // 向其中 Timer 添加 TimerEvent

    eventloop.loop();
    return 0;
}