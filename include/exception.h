#ifndef WY_EXCEPTION_H
#define WY_EXCEPTION_H

#include <exception>
#include <string>

namespace wynet
{

class Exception : public std::exception
{
public:
  explicit Exception(const char *what);
  explicit Exception(const std::string &what);
  virtual ~Exception() noexcept;
  virtual const char *what() const noexcept;
  const char *stackTrace() const noexcept;

private:
  void fillStackTrace();

  std::string m_message;
  std::string m_stack;
};
}

#endif
