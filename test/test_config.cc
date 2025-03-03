#include "rapidrpc/common/config.h"
#include "rapidrpc/common/log.h"

#include <iostream>
#include <thread>

using namespace std;

int main() {

    // set config
    rapidrpc::Config::SetGlobalConfig(nullptr);
    rapidrpc::Logger::InitGlobalLogger();

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

    rapidrpc::Logger::GetGlobalLogger()->flushAndStop();
    return 0;
}