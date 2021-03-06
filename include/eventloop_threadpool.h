#ifndef WY_EVENTLOOP_THREADPOOL_H
#define WY_EVENTLOOP_THREADPOOL_H

#include "noncopyable.h"
#include "common.h"

namespace wynet
{

class EventLoop;
class EventLoopThread;
typedef std::function<void(EventLoop *)> ThreadInitCallback;

class EventLoopThreadPool : Noncopyable
{
  public:
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);

    ~EventLoopThreadPool();

    void setThreadNum(int n);

    void start(const ThreadInitCallback &cb = {});

    // must after calling start()
    EventLoop *getNextLoop();

    // must after calling start()
    EventLoop *getLoopByHash(size_t hashCode);

    std::vector<EventLoop *> getAllLoops();

    bool isStarted() const
    {
        return m_started;
    }

    const std::string &name() const
    {
        return m_name;
    }

  private:
    EventLoop *m_baseLoop;
    std::string m_name;
    bool m_started;
    int m_numThreads;
    int m_next;
    std::vector<EventLoopThread *> m_threads;
    std::vector<EventLoop *> m_loops;
};

typedef std::shared_ptr<EventLoopThreadPool> PtrThreadPool;
};

#endif