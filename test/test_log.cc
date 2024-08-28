#include "rapidrpc/common/log.h"
#include "rapidrpc/common/config.h"
#include <iostream>
#include <thread>

using namespace std;
int main() {
    rapidrpc::Config::SetGlobalConfig(nullptr);
    rapidrpc::Logger::InitGlobalLogger();

    thread t([]() {
        DEBUGLOG("hello world in %s", "lambda thread");
    });

    // cout << logEvent.toString() << endl;
    DEBUGLOG("hello world %s", "rapidrpc");
    t.join();

    rapidrpc::Logger::GetGlobalLogger()->flushAndStop();
    return 0;
}