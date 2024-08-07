#include "net/timer.h"
#include "common/log.h"
#include "common/util.h"

#include <sys/timerfd.h> // timerfd_create
#include <string.h>      // strerror

namespace rapidrpc {

Timer::Timer() : FdEvent() {

    // create timer fd
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0) {
        ERRORLOG("create Timer fd failed");
        return;
    }
    m_fd = fd;
    DEBUGLOG("create Timer fd: %d", m_fd);

    // 设置定时器fd 的监听事件为可读，和回调函数(当前对象的onTimer)
    listen(FdEvent::TriggerEvent::IN_EVENT, std::bind(&Timer::onTimer, this));
}

Timer::~Timer() {}

// TODO: 优化
void Timer::onTimer() {
    // LT 模式，清空 timerfd 的事件
    uint64_t exp; // 超时次数
    while ((read(m_fd, &exp, sizeof(exp)) == -1) && errno == EAGAIN)
        ;
    // 比较当前时间
    int64_t now = getNowMs();
    std::vector<TimerEvent::s_ptr> events;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_pending_events.begin();
        while (it != m_pending_events.end() && it->first <= now) {
            // TODO: 读写冲突，是否需要加锁
            if (!it->second->isCancled()) {
                events.push_back(it->second);
            }
            it++;
        }
        m_pending_events.erase(m_pending_events.begin(), it); // 指针形式不会影响已有的迭代器失效
    }
    // 由于清掉了部分任务，重置 timerfd 的唤醒时间
    resetTimer();

    // 对于重复任务，重新添加到定时任务中
    for (auto &event : events) {
        // !! 取之前已经判断过 isCancled
        if (event->isRepeat()) {
            event->resetArriveTime();
            addTimerEvent(event);
        }
    }
    // 执行定时任务
    for (auto &event : events) {
        if (event->getTask()) {
            event->getTask()();
        }
    }
}

void Timer::resetTimer() {
    // 设置 timerfd 的唤醒时间, 即将最近的到达时间设置为 timerfd 的唤醒时间
    // 发生在添加定时任务时/删除定时任务时
    std::pair<int64_t, TimerEvent::s_ptr> tmp;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_pending_events.empty()) {
            // stop timerfd (优化删除时停用timerfd)
            struct itimerspec new_value;
            memset(&new_value, 0, sizeof(new_value)); // stop timerfd
            int rt = timerfd_settime(m_fd, 0, &new_value, nullptr);
            if (rt < 0) {
                ERRORLOG("timerfd_settime failed, error [%s]", strerror(errno));
            }
            return;
        }
        tmp = *m_pending_events.begin();
    }
    int64_t now = getNowMs();
    int64_t interval = 100; // 100ms 立即执行的间隔时间
    if (tmp.first > now) {
        interval = tmp.first - now;
    }
    // 设置 timerfd 的唤醒时间
    struct itimerspec new_value;
    new_value.it_interval.tv_sec = new_value.it_interval.tv_nsec = 0; // no repeat
    new_value.it_value.tv_sec = interval / 1000;
    new_value.it_value.tv_nsec = (interval % 1000) * 1000000;
    int rt = timerfd_settime(m_fd, 0, &new_value, nullptr); // flags = 0 默认相对当前的调用时间
    if (rt < 0) {
        ERRORLOG("timerfd_settime failed");
    }
}

void Timer::addTimerEvent(TimerEvent::s_ptr event) {
    // 每次添加定时任务时，都会将定时任务插入到 pending_events 中
    // !! 每次添加任务时，需要更新 timerfd 的唤醒时间，即将最近的到达时间设置为 timerfd 的唤醒时间
    bool is_reset_timerfd = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_pending_events.empty()) {
            is_reset_timerfd = true;
        }
        else {
            int64_t arrive_time = m_pending_events.begin()->first;
            if (event->getArriveTime() < arrive_time) {
                is_reset_timerfd = true;
            }
        }

        int64_t arrive_time = event->getArriveTime();
        // m_pending_events.insert({arrive_time, event});
        m_pending_events.emplace(arrive_time, event);
    }

    if (is_reset_timerfd) {
        resetTimer();
    }

    DEBUGLOG("add TimerEvent, arrive_time: %lld [%s]", event->getArriveTime(),
             getFormatTime(event->getArriveTime()).c_str());
}

void Timer::deleteTimerEvent(TimerEvent::s_ptr event) {
    // 删除定时任务
    // TODO: cancle 这里一写一读，这里是否需要加锁，cancle 仅仅用于标记吗
    event->setCancled(true);

    bool is_reset_timerfd = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto range_events = m_pending_events.equal_range(event->getArriveTime());
        for (auto it = range_events.first; it != range_events.second; ++it) {
            if (it->second == event) { // 指向同一个对象
                m_pending_events.erase(it);
                // 如果删除的是最近的到达时间，需要更新 timerfd 的唤醒时间
                if (m_pending_events.empty() || m_pending_events.begin()->first != event->getArriveTime()) {
                    is_reset_timerfd = true;
                }
                break;
            }
        }
    }
    // 如果删除的是最近的到达时间，需要更新 timerfd 的唤醒时间
    if (is_reset_timerfd) {
        resetTimer();
    }

    DEBUGLOG("delete TimerEvent, arrive_time: %lld [%s]", event->getArriveTime(),
             getFormatTime(event->getArriveTime()).c_str());
}

} // namespace rapidrpc