/**
 * 作为客户端，测试 test_tcpserver.cc
 */

#include "net/tcp/net_addr.h"
#include "common/log.h"
#include "common/config.h"
#include <unistd.h>

int main() {
    rapidrpc::Config::SetGlobalConfig("/home/xiao/rapidrpc/rapidrpc/conf/rapidrpc.xml");

    // ipv4
    // rapidrpc::IpNetAddr ipNetAddr("0.0.0.0:12345");
    rapidrpc::IpNetAddr serverAddr("198.19.249.138:12345");

    // ===========================================================
    int clifd = socket(AF_INET, SOCK_STREAM, 0);
    if (clifd < 0) {
        ERRORLOG("Failed to create socket");
        return -1;
    }

    // struct sockaddr_in server = *((sockaddr_in *)serverAddr.getSockAddr());
    auto addr = serverAddr.getSockAddr();
    auto addr_len = serverAddr.getSockAddrLen();
    int ret = connect(clifd, addr, addr_len);

    if (ret < 0) {
        ERRORLOG("Failed to connect to server");
        return -1;
    }

    INFOLOG("Connect to server success");

    for (int i = 0; i < 10; i++) {
        char buf[1024] = "hello, world";
        int n = write(clifd, buf, strlen(buf));
        if (n < 0) {
            ERRORLOG("Failed to write to server");
            return -1;
        }
        INFOLOG("Write to server success");
        // receive
        char recv_buf[1024];
        n = read(clifd, recv_buf, 1024);
        if (n < 0) {
            ERRORLOG("Failed to read from server");
            return -1;
        }
        recv_buf[n] = '\0';
        INFOLOG("Read from server success, content: %s", recv_buf);
        sleep(2);
    }

    close(clifd);
    return 0;
}