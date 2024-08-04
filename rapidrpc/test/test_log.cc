#include "common/log.h"
#include <iostream>
#include <thread>

using namespace std;
int main() {

    thread t([]() {
        DEBUGLOG("hello world in %s", "lambda thread");
    });

    // cout << logEvent.toString() << endl;
    DEBUGLOG("hello world %s", "rapidrpc");
    t.join();

    return 0;
}