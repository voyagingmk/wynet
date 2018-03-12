#ifndef WY_EVENT_LOOP_H
#define WY_EVENT_LOOP_H

#include "noncopyable.h"
#include "common.h"

namespace wynet
{

class EventLoop : Noncopyable
{
  public:
    typedef void (*OnFileEvent)(EventLoop *, int fd, void *userData, int mask);
    typedef int (*OnTimerEvent)(EventLoop *, int timerfd, void *userData);

    EventLoop(int defaultSetsize = 64);

    ~EventLoop();

    void loop();

    void stop();

    void createFileEvent(int fd, int mask, OnFileEvent onFileEvent, void *userData);

    void deleteFileEvent(int fd, int mask);

    long long createTimer(long long ms, OnTimerEvent onTimerEvent, void *userData);

    bool deleteTimer(long long timerid);

  public:
    struct FDData
    {
        FDData() : onFileEvent(nullptr),
                   userDataRead(nullptr),
                   userDataWrite(nullptr)
        {
        }
        OnFileEvent onFileEvent;
        void *userDataRead;
        void *userDataWrite;
    };

    struct TimerData
    {
        TimerData() : onTimerEvent(nullptr),
                      userData(nullptr)
        {
        }
        OnTimerEvent onTimerEvent;
        void *userData;
    };

  private:
    friend void aeOnFileEvent(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
    friend int aeOnTimerEvent(struct aeEventLoop *eventLoop, long long timerid, void *clientData);

    aeEventLoop *aeloop;
    std::map<int, std::shared_ptr<FDData>> fdData;
    std::map<long long, std::shared_ptr<TimerData>> timerData;
};
};

#endif