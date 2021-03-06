#include "logger/log_file.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"
#include "logger/append_file.h"

using namespace wynet;
using namespace std;

static string getLogFileName(const string &basename, time_t *now);

LogFile::LogFile(const string &basename,
                 off_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
    : m_basename(basename),
      m_rollSize(rollSize),
      m_flushInterval(flushInterval),
      m_checkEveryN(checkEveryN),
      m_logLinesCount(0),
      m_mutex(threadSafe ? new MutexLock : nullptr),
      m_startPeriod(0),
      m_lastRoll(0),
      m_lastFlush(0)
{
}

LogFile::~LogFile()
{
}

void LogFile::append(const char *logline, int len)
{
  if (m_mutex)
  {
    MutexLockGuard<MutexLock> lock(*m_mutex);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}

void LogFile::flush()
{
  if (m_mutex)
  {
    MutexLockGuard<MutexLock> lock(*m_mutex);
    getAppendFile()->flush();
  }
  else
  {
    getAppendFile()->flush();
  }
}

void LogFile::append_unlocked(const char *logline, int len)
{
  getAppendFile()->append(logline, len);

  if (getAppendFile()->writtenBytes() > m_rollSize)
  {
    rollFile();
  }
  else
  {
    ++m_logLinesCount;
    if (m_logLinesCount >= m_checkEveryN)
    {
      m_logLinesCount = 0;
      time_t now = ::time(NULL);
      time_t curPeriod = getPeriod(now);
      if (curPeriod != m_startPeriod)
      {
        rollFile();
      }
      else if (now - m_lastFlush > m_flushInterval)
      {
        m_lastFlush = now;
        getAppendFile()->flush();
      }
    }
  }
}

unique_ptr<AppendFile> &LogFile::getAppendFile()
{
  if (!m_file)
  {
    rollFile();
  }
  return m_file;
}

void LogFile::rollFile()
{
  time_t now = 0;
  assert(m_basename.find('/') == string::npos);
  string filename = getLogFileName(m_basename, &now);
  time_t startPeriod = getPeriod(now);
  if (now > m_lastRoll)
  {
    m_lastRoll = now;
    m_lastFlush = now;
    m_startPeriod = startPeriod;
    m_file.reset(new AppendFile(filename));
  }
}

string getLogFileName(const string &basename, time_t *now)
{
  string filename;
  {
    // basesame
    filename.reserve(basename.size() + 64);
    filename = basename;
  }
  {
    // basesame + .utctime.
    struct tm tm;
    ::time(now);
    gmtime_r(now, &tm); // thread-safe
    char timebuf[32] = {0};
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    // e.g.：20180422-133335
    filename += timebuf;
  }
  {
    // basesame + .utctime. + hostname
    filename += hostname();
  }
  {
    // basesame + .utctime. + hostname + .pid
    char pidbuf[32];
    snprintf(pidbuf, sizeof pidbuf, ".%d", getpid());
    filename += pidbuf;
  }
  {
    // basesame + .utctime. + hostname + .pid + .log
    filename += ".log";
  }

  return filename;
}
