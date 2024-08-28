#include "rapidrpc/net/timer_event.h"
#include "rapidrpc/common/util.h"
#include "rapidrpc/common/log.h"

#include <chrono>

namespace rapidrpc {

TimerEvent::TimerEvent(int interval, bool is_repeat, std::function<void()> cb)
    : m_interval(interval), m_is_repeat(is_repeat), m_task(cb) {

    // m_arrive_time = getNowMs() + m_interval;
    resetArriveTime();

    DEBUGLOG("create TimerEvent, arrive_time: %lld [%s], interval: %d, repeat:%s", m_arrive_time,
             getFormatTime(m_arrive_time).c_str(), m_interval, m_is_repeat ? "true" : "false");
}

TimerEvent::~TimerEvent() {}

void TimerEvent::resetArriveTime() {
    // m_arrive_time += m_interval;
    // ! 使用 NowMs 纠正
    m_arrive_time = getNowMs() + m_interval;
}

} // namespace rapidrpc