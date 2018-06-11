#ifndef WY_MULTIPLEXING_H
#define WY_MULTIPLEXING_H

#include "common.h"

namespace wynet
{

#define MP_OK 0
#define MP_ERR -1

#define MP_NONE 0
#define MP_READABLE 1
#define MP_WRITABLE 2

#define MP_FILE_EVENTS 1
#define MP_TIME_EVENTS 2
#define MP_ALL_EVENTS (MP_FILE_EVENTS | MP_TIME_EVENTS)
#define MP_DONT_WAIT 4
#define MP_CALL_AFTER_SLEEP 8

#define MP_NOMORE -1
#define MP_DELETED_EVENT_ID -1

#define MP_NOTUSED(V) ((void)V)

class MpEventLoop;

using MpFileProc = std::function<void(MpEventLoop *eventLoop, int fd, void *clientData, int mask)>;

using MpTimeProc = std::function<int(MpEventLoop *eventLoop, long long id, void *clientData)>;

using MpEventFinalizerProc = std::function<void(MpEventLoop *eventLoop, void *clientData)>;

using MpBeforeSleepProc = std::function<void(MpEventLoop *eventLoop)>;

struct MpFileEvent
{
    MpFileEvent() : mask(MP_NONE),
                    rfileProc(nullptr),
                    wfileProc(nullptr),
                    clientData(nullptr)
    {
    }
    int mask;
    MpFileProc rfileProc;
    MpFileProc wfileProc;
    void *clientData;
};

struct MpTimeEvent
{
    long long id;
    long when_sec;
    long when_ms;
    MpTimeProc timeProc;
    MpEventFinalizerProc finalizerProc;
    void *clientData;
    bool operator<(const MpTimeEvent &rhs) const;
};

typedef std::shared_ptr<MpTimeEvent> MpTimeEventPtr;

struct Compare
{
    bool operator()(const MpTimeEventPtr &a, const MpTimeEventPtr &b)
    {
        return *a < *b;
    }
};

struct MpFiredEvent
{
    int fd;
    int mask;
};

// not thread-safe
class MpEventLoop
{
  public:
    MpEventLoop(int setsize);

    ~MpEventLoop();

    void stop();

    int resizeSetSize(int setsize);

    int createFileEvent(int fd, int mask,
                        MpFileProc proc, void *clientData);

    void deleteFileEvent(int fd, int mask);

    int getFileEvents(int fd);

    long long createTimeEvent(long long milliseconds,
                              MpTimeProc proc, void *clientData,
                              MpEventFinalizerProc finalizerProc);

    int deleteTimeEvent(long long id);

    int processEvents(int flags);

    const MpTimeEventPtr &searchNearestTimer();

    int wait(int fd, int mask, long long milliseconds);

    void main();

    void setBeforeSleepProc(MpBeforeSleepProc beforesleep);

    void setAfterSleepProc(MpBeforeSleepProc aftersleep);

    inline const char *getApiName(void);

    inline int getSetSize() { return m_setsize; }

    inline void *getApiData() { return m_apidata; }

    inline void setApiData(void *apidata) { m_apidata = apidata; }

    inline int getMaxFd() { return m_maxfd; }

    inline std::vector<MpFileEvent> &getEvents() { return m_events; }

    inline std::vector<MpFiredEvent> &getFiredEvents() { return m_fired; }

  private:
    int processTimeEvents();

  private:
    int m_maxfd;
    int m_setsize;
    long long m_timeEventNextId;
    time_t m_lastTime;
    std::vector<MpFileEvent> m_events;
    std::vector<MpFiredEvent> m_fired;
    MpTimeEventPtr m_timeEventNearest;
    int m_stop;
    void *m_apidata;
    std::multiset<MpTimeEventPtr, Compare> m_teSet;
    MpBeforeSleepProc m_beforesleep;
    MpBeforeSleepProc m_aftersleep;
};

}; // namespace wynet
#endif
