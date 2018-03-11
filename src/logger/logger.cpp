
#include "logger/logger.h"

namespace wynet
{
Logger::Logger(const string &logtitle,
               off_t rollSize,
               int flushInterval) : m_flushInterval(flushInterval),
                                    m_running(false),
                                    m_logtitle(logtitle),
                                    m_rollSize(rollSize),
                                    m_thread(std::bind(&Logger::threadFunc, this), "Logging"),
                                    m_latch(1),
                                    m_mutex(),
                                    m_cond(m_mutex),
                                    m_curBuffer(new LoggingBuffer),
                                    m_nextBuffer(new LoggingBuffer),
                                    m_buffers()
{
    m_curBuffer->clean();
    m_nextBuffer->clean();
    m_buffers.reserve(16);
}

void Logger::append(const char *logline, int len)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    if (m_curBuffer->leftOpacity() > len)
    {
        m_curBuffer->append(logline, len);
    }
    else
    {
        m_buffers.push_back(m_curBuffer);

        if (m_nextBuffer)
        {
            m_curBuffer = std::move(m_nextBuffer);
        }
        else
        {
            m_curBuffer.reset(new LoggingBuffer()); // Rarely happens
        }
        m_curBuffer->append(logline, len);
        m_cond.notify();
    }
}

void Logger::threadFunc()
{
    assert(m_running == true);
    m_latch.countDown();
    LogFile output(m_logtitle, m_rollSize, false);
    BufferPtr newBuffer1(new LoggingBuffer);
    BufferPtr newBuffer2(new LoggingBuffer);
    newBuffer1->clean();
    newBuffer2->clean();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (m_running)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            MutexLockGuard<MutexLock> lock(mutex);
            if (m_buffers.empty()) // unusual usage!
            {
                m_cond.waitForSeconds(m_flushInterval);
            }
            m_buffers.push_back(currentBuffer_.release());
            m_currentBuffer = std::move(newBuffer1);
            buffersToWrite.swap(m_buffers);
            if (!m_nextBuffer)
            {
                m_nextBuffer = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());

        if (buffersToWrite.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::now().toFormattedString().c_str(),
                     buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        for (size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffersToWrite[i].data(), buffersToWrite[i].length());
        }

        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}
};
