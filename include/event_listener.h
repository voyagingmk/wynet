#ifndef WY_EVENT_LISTENER_H
#define WY_EVENT_LISTENER_H

#include "common.h"
#include "timer_ref.h"

namespace wynet
{

class TimerRef;
class EventLoop;
class EventListener;

using PtrEvtListener = std::shared_ptr<EventListener>;
using WeakPtrEvtListener = std::weak_ptr<EventListener>;
using OnFileEvent = std::function<void(EventLoop *, PtrEvtListener listener, int mask)>;
using OnTimerEvent = std::function<int(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)>;

// 用来发起事件监听
// 触发~EventListener()时，从eventloop删除相关的事件，保证不会有无用的事件监听
// 必须要用shared_ptr
// Note: 如果创建了多个指向同个sockfd的EventListener，会有问题
// 可以被继承，就能够绑定更多的参数
class EventListener : public std::enable_shared_from_this<EventListener>
{

  public:
    EventListener();

    virtual ~EventListener();

  public:
    // 可重载
    static PtrEvtListener create()
    {
        return std::make_shared<EventListener>();
    }

    void setSockfd(SockFd sockfd)
    {
        m_sockfd = sockfd;
    }

    void setName(std::string name)
    {
        m_name = name;
    }

    const std::string &getName()
    {
        return m_name;
    }

    void setEventLoop(EventLoop *loop)
    {
        m_loop = loop;
    }

    SockFd getSockFd()
    {
        return m_sockfd;
    }

    EventLoop *getEventLoop()
    {
        return m_loop;
    }

    int getFileEventMask() { return m_mask; }

    bool hasFileEvent(int mask);

    void deleteAllFileEvent();

    void createFileEvent(int mask, OnFileEvent onFileEvent);

    void deleteFileEvent(int mask);

    TimerRef createTimer(int ms, OnTimerEvent onTimerEvent, void *data);

  public:
    EventLoop *m_loop;
    SockFd m_sockfd;
    std::string m_name;
    OnFileEvent m_onFileEvent;
    int m_mask;
};
}; // namespace wynet

#endif