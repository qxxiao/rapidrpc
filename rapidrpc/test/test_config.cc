#include "common/config.h"
#include "common/log.h"

#include <iostream>
#include <thread>

using namespace std;

int main() {

    // set config
    rapidrpc::Config::SetGlobalConfig("/home/xiao/rapidrpc/rapidrpc/conf/rapidrpc.xml");

    // 5 threads
    vector<thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.push_back(thread([i]() {
            DEBUGLOG("thread-%d debug: hello world in %s", i, "lambda thread");
            INFOLOG("thread-%d info: hello world in %s", i, "lambda thread");
            ERRORLOG("thread-%d error: hello world in %s", i, "lambda thread");
        }));
    }
    // join
    for (auto &t : threads) {
        t.join();
    }
    return 0;
}