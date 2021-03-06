#include "eventloop.h"

namespace wynet
{

// thread local
__thread EventLoop *t_threadLoop = nullptr;
std::atomic<WyTimerId> TimerRef::g_numCreated;

int OnlyForWakeup(EventLoop *, TimerRef tr, std::weak_ptr<FDRef> fdRef, void *data)
{
    const int *ms = (const int *)(data);
    return *ms;
}

void aeOnFileEvent(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    EventLoop *loop = (EventLoop *)(clientData);
    EventLoop::FDData &p = loop->m_fdData[fd];
    if (p.onFileEvent)
    {
        if ((mask & LOOP_EVT_READABLE) || (mask & LOOP_EVT_WRITABLE))
        {
            p.onFileEvent(loop, p.fdRef, mask);
        }
    }
}

int aeOnTimerEvent(struct aeEventLoop *eventLoop, AeTimerId aeTimerId, void *clientData)
{
    EventLoop *loop = (EventLoop *)(clientData);
    TimerRef tr = loop->m_aeTimerId2ref[aeTimerId];
    do
    {
        if (!tr.validate())
        {
            break;
        }
        if (loop->m_timerData.find(tr) == loop->m_timerData.end())
        {
            break;
        }
        EventLoop::TimerData &td = loop->m_timerData[tr];
        if (!td.onTimerEvent)
        {
            loop->m_timerData.erase(tr);
            break;
        }
        int ret = td.onTimerEvent(loop, tr, td.fdRef, td.data);
        if (ret == AE_NOMORE)
        {
            break;
        }
        return ret;
    } while (0);
    loop->m_aeTimerId2ref.erase(aeTimerId);
    return AE_NOMORE;
}

EventLoop::EventLoop(int wakeupInterval, int defaultSetsize) : m_threadId(CurrentThread::tid()),
                                                               m_wakeupInterval(wakeupInterval),
                                                               m_doingTask(false)
{
    if (t_threadLoop)
    {
        log_fatal("Create 2 EventLoop For 1 thread?");
    }
    else
    {
        t_threadLoop = this;
    }
    m_aeloop = aeCreateEventLoop(defaultSetsize);
}

EventLoop::~EventLoop()
{
    aeDeleteEventLoop(m_aeloop);
    t_threadLoop = nullptr;
}

void EventLoop::loop()
{
    assertInLoopThread();
    m_aeloop->stop = 0;
    AeTimerId aeTimerId = createTimerInLoop(m_wakeupInterval, OnlyForWakeup, std::weak_ptr<FDRef>(), (void *)&m_wakeupInterval);
    while (!m_aeloop->stop)
    {
        if (m_aeloop->beforesleep != NULL)
            m_aeloop->beforesleep(m_aeloop);
        aeProcessEvents(m_aeloop, AE_ALL_EVENTS | AE_CALL_AFTER_SLEEP);
        processTaskQueue();
    }
    deleteTimerInLoop(aeTimerId);
}

void EventLoop::stop()
{
    if (!m_aeloop->stop)
    {
        aeStop(m_aeloop);
        log_info("EventLoop stop");
    }
}

void EventLoop::createFileEvent(std::shared_ptr<FDRef> fdRef, int mask, OnFileEvent onFileEvent)
{
    assert((mask & AE_READABLE) || (mask & AE_WRITABLE));

    int setsize = aeGetSetSize(m_aeloop);
    int fd = fdRef->fd();
    while (fd >= setsize)
    {
        assert(AE_ERR != aeResizeSetSize(m_aeloop, setsize << 1));
    }
    int ret = aeCreateFileEvent(m_aeloop, fd, mask, aeOnFileEvent, (void *)this);
    assert(AE_ERR != ret);
    m_fdData[fd] = FDData(onFileEvent, fdRef);
}

void EventLoop ::deleteFileEvent(std::shared_ptr<FDRef> fdRef, int mask)
{
    int fd = fdRef->fd();
    deleteFileEvent(fd, mask);
}

void EventLoop ::deleteFileEvent(int fd, int mask)
{
    int setsize = aeGetSetSize(m_aeloop);
    if (fd >= setsize)
    {
        log_warn("deleteFileEvent fd >= setsize  %d %d\n", fd, setsize);
    }
    aeDeleteFileEvent(m_aeloop, fd, mask);
    if (!aeGetFileEvents(m_aeloop, fd) && m_fdData.find(fd) != m_fdData.end())
    {
        m_fdData.erase(fd);
    }
}

TimerRef EventLoop ::createTimer(int ms, OnTimerEvent onTimerEvent, std::weak_ptr<FDRef> fdRef, void *data)
{
    TimerRef tr = TimerRef::newTimerRef();
    runInLoop([&]() {
        createTimerInLoop(tr, ms, onTimerEvent, fdRef, data);
    });
    return tr;
}

void EventLoop ::deleteTimer(TimerRef tr)
{
    runInLoop([&]() {
        deleteTimerInLoop(tr);
    });
}

bool EventLoop ::deleteTimerInLoop(AeTimerId aeTimerId)
{
    assertInLoopThread();
    auto it = m_aeTimerId2ref.find(aeTimerId);
    if (it == m_aeTimerId2ref.end()) {
        return false;
    }
    TimerRef tr = it->second;
    return deleteTimerInLoop(tr);
}

bool EventLoop ::deleteTimerInLoop(TimerRef tr)
{
    assertInLoopThread();
    if (m_timerData.find(tr) != m_timerData.end())
    {
        WyTimerId timerId = m_timerData[tr].timerId;
        m_timerData.erase(tr);
        int ret = aeDeleteTimeEvent(m_aeloop, timerId);
        return AE_ERR != ret;
    }
    return false;
}

AeTimerId EventLoop ::createTimerInLoop(int delay, OnTimerEvent onTimerEvent, std::weak_ptr<FDRef> fdRef, void *data)
{
    TimerRef tr = TimerRef::newTimerRef();
    return createTimerInLoop(tr, delay, onTimerEvent, fdRef, data);
}

AeTimerId EventLoop ::createTimerInLoop(TimerRef tr, int ms, OnTimerEvent onTimerEvent, std::weak_ptr<FDRef> fdRef, void *data)
{
    assertInLoopThread();
    AeTimerId aeTimerId = aeCreateTimeEvent(m_aeloop, ms, aeOnTimerEvent, (void *)this, NULL);
    assert(AE_ERR != aeTimerId);
    assert(m_timerData.find(tr) == m_timerData.end());
    m_timerData[tr] = {onTimerEvent, fdRef, data, aeTimerId};
    m_aeTimerId2ref.insert({aeTimerId, tr});
    return aeTimerId;
}

void EventLoop::runInLoop(const TaskFunction &cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const TaskFunction &cb)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    m_taskFuncQueue.push_back(cb);
}

size_t EventLoop::queueSize() const
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    return m_taskFuncQueue.size();
}

void EventLoop::processTaskQueue()
{
    std::vector<TaskFunction> taskFuncQueue;
    m_doingTask = true;

    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        taskFuncQueue.swap(m_taskFuncQueue);
    }

    for (size_t i = 0; i < taskFuncQueue.size(); ++i)
    {
        taskFuncQueue[i]();
    }
    m_doingTask = false;
}

EventLoop *EventLoop::getCurrentThreadLoop()
{
    return t_threadLoop;
}
}; // namespace wynet
